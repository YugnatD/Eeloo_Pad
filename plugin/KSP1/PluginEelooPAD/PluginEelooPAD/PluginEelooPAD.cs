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
        private static Socket serverSocket;
        private static Socket clientSocket;
        private XmlDocument xmlConfig = new XmlDocument();
        private static IPHostEntry ipHost;
        private static IPAddress ipAddr;
        private static IPEndPoint localEndPoint;

        public static int refreshRate = 100; // in ms

        public static DataToSend packet; // Will contain data of the vessel

        public static FlightCtrlState actualControlState = new FlightCtrlState();

        private static bool init = false;
        
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

        public static void SendVesselInfo()
        {
            // send data to socket
            byte[] messageSent = UtilityEeloo.StructureToByteArray(packet);
            if (clientSocket.Connected)
            {
                int byteSent = clientSocket.Send(messageSent);
            }
        }

        private static void threadSocketClient()
        {
            Debug.Log("[EelooPad] Waiting for a client");
            clientSocket = serverSocket.Accept();
            Debug.Log("[EelooPad] Got a connection");
            while(clientSocket.Connected)
            {
                //read data from the socket
                byte[] messageReceived = new byte[256];
                int byteRecv = clientSocket.Receive(messageReceived);
                
                if (byteRecv == 0)
                {
                    Debug.Log("[EelooPad] Client disconnected");
                    clientSocket.Close();
                    clientSocket = serverSocket.Accept();// wait for a new connection
                    Debug.Log("[EelooPad] Got a new connection");
                }
                else
                {
                    // keep only the received byte :
                    byte[] toDecode = new byte[byteRecv];
                    Array.Copy(messageReceived, toDecode, byteRecv);
                   //cast the data to a structure
                    VesselControls controls = UtilityEeloo.MessageToControls(toDecode);
                    FlightGlobals.ActiveVessel.ActionGroups.SetGroup(KSPActionGroup.SAS, controls.SAS);
                    FlightGlobals.ActiveVessel.ActionGroups.SetGroup(KSPActionGroup.RCS, controls.RCS);
                    FlightGlobals.ActiveVessel.ActionGroups.SetGroup(KSPActionGroup.Light, controls.Lights);
                    FlightGlobals.ActiveVessel.ActionGroups.SetGroup(KSPActionGroup.Gear, controls.Gear);
                    FlightGlobals.ActiveVessel.ActionGroups.SetGroup(KSPActionGroup.Brakes, controls.Brakes);

                    FlightGlobals.ActiveVessel.ActionGroups.SetGroup(KSPActionGroup.Stage, controls.Stage);
                    
                    actualControlState.mainThrottle = controls.Throttle;
                    actualControlState.pitch = controls.Pitch;
                    actualControlState.yaw = controls.Yaw;
                    actualControlState.roll = controls.Roll;
                    actualControlState.X = controls.TX;
                    actualControlState.Y = controls.TY;
                    actualControlState.Z = controls.TZ;

                    //Stage
                    
                }
            }
        }
    }

    
}