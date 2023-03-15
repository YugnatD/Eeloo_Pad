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
        
        void Awake() //called on startup of the plugin by Unity
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
            //clientSocket = serverSocket.Accept();
        }

        public static void SendStruct()
        {
            // send data to socket
            byte[] messageSent = StructureToByteArray(packet);
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
                    Debug.Log("[EelooPad] Message received : " + Encoding.ASCII.GetString(messageReceived));
                }
            }
        }

        //THANKS KSPSERIALIO
        private static byte[] StructureToByteArray(object obj)
        {
            int len = Marshal.SizeOf(obj);
            byte[] arr = new byte[len];
            IntPtr ptr = Marshal.AllocHGlobal(len);
            Marshal.StructureToPtr(obj, ptr, true);
            Marshal.Copy(ptr, arr, 0, len);
            Marshal.FreeHGlobal(ptr);
            return arr;
        }

        private static object ByteArrayToStructure(byte[] bytearray, object obj)
        {
            int len = Marshal.SizeOf(obj);
            IntPtr i = Marshal.AllocHGlobal(len);
            Marshal.Copy(bytearray, 0, i, len);
            obj = Marshal.PtrToStructure(i, obj.GetType());
            Marshal.FreeHGlobal(i);
            return obj;
        }
    }

    
}