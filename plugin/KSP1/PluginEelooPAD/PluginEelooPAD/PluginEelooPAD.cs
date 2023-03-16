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
                byte[] messageReceived = new byte[1024];
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
                   //cast the data to a structure
                    VesselControls controls = UtilityEeloo.MessageToControls(messageReceived);
                    FlightGlobals.ActiveVessel.ActionGroups.SetGroup(KSPActionGroup.SAS, controls.SAS);
                    FlightGlobals.ActiveVessel.ActionGroups.SetGroup(KSPActionGroup.RCS, controls.RCS);
                    FlightGlobals.ActiveVessel.ActionGroups.SetGroup(KSPActionGroup.Light, controls.Lights);
                    FlightGlobals.ActiveVessel.ActionGroups.SetGroup(KSPActionGroup.Gear, controls.Gear);
                    FlightGlobals.ActiveVessel.ActionGroups.SetGroup(KSPActionGroup.Brakes, controls.Brakes);
                    FlightGlobals.ActiveVessel.ctrlState.mainThrottle = controls.Throttle;
                    FlightGlobals.ActiveVessel.ctrlState.pitch = controls.Pitch;
                    FlightGlobals.ActiveVessel.ctrlState.yaw = controls.Yaw;
                    FlightGlobals.ActiveVessel.ctrlState.roll = controls.Roll;
                    FlightGlobals.ActiveVessel.ctrlState.X = controls.TX;
                    FlightGlobals.ActiveVessel.ctrlState.Y = controls.TY;
                    FlightGlobals.ActiveVessel.ctrlState.Z = controls.TZ;
                    // print the structure to the log
                    Debug.Log("[EelooPad] Throttle : " + controls.Throttle);
                    Debug.Log("[EelooPad] Pitch : " + controls.Pitch);
                    Debug.Log("[EelooPad] Yaw : " + controls.Yaw);
                    Debug.Log("[EelooPad] Roll : " + controls.Roll);
                    Debug.Log("[EelooPad] TX : " + controls.TX);
                    Debug.Log("[EelooPad] TY : " + controls.TY);
                    Debug.Log("[EelooPad] TZ : " + controls.TZ);
                    Debug.Log("[EelooPad] SAS : " + controls.SAS);
                    Debug.Log("[EelooPad] RCS : " + controls.RCS);
                    Debug.Log("[EelooPad] Lights : " + controls.Lights);
                    Debug.Log("[EelooPad] Gear : " + controls.Gear);
                    Debug.Log("[EelooPad] Brakes : " + controls.Brakes);
                    Debug.Log("[EelooPad] Stage : " + controls.Stage);
                    //Stage
                    FlightGlobals.ActiveVessel.ActionGroups.SetGroup(KSPActionGroup.Stage, true);
                }
            }
        }
    }

    
}