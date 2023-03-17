/****************************************************************************************************
*** Author : Tanguy Dietrich / Kirill Goundiaev
*** Name : PluginEelooPAD.cs
*** Description : Will init the socket and launche the thread who will manage the connection on the socket
***              And manage the incoming data from the socket to update the vessel state
*****************************************************************************************************/

using System;
using System.Net;
using System.Net.Sockets;
using System.Text;
using System.Collections.Generic;
using System.Linq;
using System.Threading;
using System.Xml;
using UnityEngine;
using System.Runtime.InteropServices;
using KSP.IO;
using KSP.UI.Screens;

namespace PluginEelooPAD
{
    [KSPAddon(KSPAddon.Startup.MainMenu, false)]
    public class EelooCom : MonoBehaviour
    {
        #region PRIVATE ATTRIBUTES
        // Private Attributes
        private static bool init = false;
        private static Socket serverSocket;
        private static Socket clientSocket;
        private XmlDocument xmlConfig = new XmlDocument();
        private static IPHostEntry ipHost;
        private static IPAddress ipAddr;
        private static IPEndPoint localEndPoint;
        #endregion
        #region PUBLIC ATTRIBUTES
        // Public Attributes
        public static int refreshRate = 100; // in ms
        public static DataToSend packet; // Will contain data of the vessel
        public static FlightCtrlState actualControlState = new FlightCtrlState();
        #endregion
        
        #region METHODS
        /****************************************************************************************************
        *** Name : Awake()
        *** Description : Init the socket and launche the thread who will manage the connection on the socket
        *** Input : None
        *** Return : None
        *****************************************************************************************************/
        void Awake() //called on startup of the plugin by Unity
        {
            if(init == false)
            {
                Debug.Log("[EelooPad] Awake Call");
                // read XML configuration file
                xmlConfig.Load("GameData/PluginEelooPAD/config.xml");
                Debug.Log("[EelooPad] XML Loaded");
                //read port from the XML file
                int port = int.Parse(xmlConfig.DocumentElement.SelectSingleNode("/config/Port").InnerText);
                Debug.Log("[EelooPad] Port : " + port.ToString());
                String ip = xmlConfig.DocumentElement.SelectSingleNode("/config/IP").InnerText;
                ipAddr = IPAddress.Parse(ip);
                Debug.Log("[EelooPad] IP : " + ip);
                refreshRate = int.Parse(xmlConfig.DocumentElement.SelectSingleNode("/config/refresh").InnerText);
                Debug.Log("[EelooPad] refreshRate : " + refreshRate.ToString());
                //init a socket to listen to
                ipHost = Dns.GetHostEntry(Dns.GetHostName());
                localEndPoint = new IPEndPoint(ipAddr, port);
                // init a socket to send data to
                serverSocket = new Socket(ipAddr.AddressFamily, SocketType.Stream, ProtocolType.Tcp);
                serverSocket.Bind(localEndPoint);
                serverSocket.Listen(1);
                //launche the thread who will manage the connection on the socket :
                Thread backgroundThread = new Thread(new ThreadStart(threadSocketClient));
                backgroundThread.Start();
                Debug.Log("[EelooPad] Awake OK");
                init = true;
                actualControlState.mainThrottle = 0;
                actualControlState.pitch = 0;
                actualControlState.roll = 0;
                actualControlState.yaw = 0;
                actualControlState.X = 0;
                actualControlState.Y = 0;
                actualControlState.Z = 0;
                actualControlState.wheelSteer = 0;
                actualControlState.wheelSteerTrim = 0;
                actualControlState.wheelThrottle = 0;
                actualControlState.wheelThrottleTrim = 0;
            }
        }

        /****************************************************************************************************
        *** Name : SendVesselInfo()
        *** Description : Send the vessel data to the socket
        *** Input : None
        *** Return : None
        *****************************************************************************************************/
        public static void SendVesselInfo()
        {
            // send data to socket
            byte[] messageSent = UtilityEeloo.StructureToByteArray(packet);
            if (clientSocket.Connected)
            {
                int byteSent = clientSocket.Send(messageSent);
            }
        }

        /****************************************************************************************************
        *** Name : threadSocketClient()
        *** Description : Thread who will manage the incoming message on the socket
                          And update the vessel control state
        *** Input : None
        *** Return : None
        *****************************************************************************************************/
        private static void threadSocketClient()
        {
            Debug.Log("[EelooPad] Waiting for a client");
            clientSocket = serverSocket.Accept();
            Debug.Log("[EelooPad] Got a connection");
            while(clientSocket.Connected)
            {
                //read data from the socket
                byte[] messageReceived = new byte[128];
                int byteRecv = clientSocket.Receive(messageReceived);
                if (byteRecv == 0)
                {
                    // no Byte received, client disconnected, close the socket
                    Debug.Log("[EelooPad] Client disconnected");
                    clientSocket.Close();
                    // wait for a new connection
                    clientSocket = serverSocket.Accept();// wait for a new connection
                    Debug.Log("[EelooPad] Got a new connection");
                }
                else
                {
                    // keep only the received byte :
                    byte[] toDecode = new byte[byteRecv];
                    Array.Copy(messageReceived, toDecode, byteRecv);
                   // cast the data to a structure
                    VesselControls controls = UtilityEeloo.MessageToControls(toDecode);
                    // Update the newly received control state and Action
                    FlightGlobals.ActiveVessel.ActionGroups.SetGroup(KSPActionGroup.SAS, controls.SAS);
                    FlightGlobals.ActiveVessel.ActionGroups.SetGroup(KSPActionGroup.RCS, controls.RCS);
                    FlightGlobals.ActiveVessel.ActionGroups.SetGroup(KSPActionGroup.Light, controls.Lights);
                    FlightGlobals.ActiveVessel.ActionGroups.SetGroup(KSPActionGroup.Gear, controls.Gear);
                    FlightGlobals.ActiveVessel.ActionGroups.SetGroup(KSPActionGroup.Brakes, controls.Brakes);
                    // NOTE : this will trigger the stage on InFlight.cs:Update()
                    // Tried to call it directly here but it didn't work, mabe a thread issue
                    FlightGlobals.ActiveVessel.ActionGroups.SetGroup(KSPActionGroup.Stage, controls.Stage); 
                    // Update the control state
                    actualControlState.mainThrottle = controls.Throttle;
                    actualControlState.pitch = controls.Pitch;
                    actualControlState.yaw = controls.Yaw;
                    actualControlState.roll = controls.Roll;
                    actualControlState.X = controls.TX;
                    actualControlState.Y = controls.TY;
                    actualControlState.Z = controls.TZ;
                }
            }
        }
        #endregion
    }

    
}