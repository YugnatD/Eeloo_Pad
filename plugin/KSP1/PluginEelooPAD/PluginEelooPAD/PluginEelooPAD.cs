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
        //private XmlDocument xmlConfig = new XmlDocument();
        private static IPHostEntry ipHost;
        private static IPAddress ipAddr;
        private static IPEndPoint localEndPoint;

        public static DataToSend packet; // Will contain data of the vessel
        //called on startup of the plugin by Unity
        void Awake()
        {
            Debug.Log("[EelooPad] Awake Call");
            ipHost = Dns.GetHostEntry(Dns.GetHostName());
            ipAddr = IPAddress.Parse("127.0.0.1"); // Loopback
            //ipAddr = ipHost.AddressList[0]; // Local IP

            // read XML configuration file
            //xmlConfig.Load("PluginEelooPAD.xml");
            // TODO: read XML configuration file
            localEndPoint = new IPEndPoint(ipAddr, 11111);
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
            //TODO NEED TO CHECK IF THERE IS A CONNECTION HERE...
            int byteSent = clientSocket.Send(messageSent);
        }

        private static void threadSocketClient()
        {
            Debug.Log("[EelooPad] On Thread Socket CLient");
            Debug.Log("[EelooPad] Waiting for a client");
            clientSocket = serverSocket.Accept();
            Debug.Log("[EelooPad] Got a connection");
            //Send a test message :
            //TODO NEED TO CLOSE THE CONNECTION IF THE CLIENT DISCONNECT AND WAIT FOR A NEW CONNECTION
            // byte[] messageSent = Encoding.ASCII.GetBytes("Test 1234");
            //clientSocket.Send(messageSent);
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