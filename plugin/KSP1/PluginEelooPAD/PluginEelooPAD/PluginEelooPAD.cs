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
        private Socket sender;
        private XmlDocument xmlConfig = new XmlDocument();
        private IPHostEntry ipHost = Dns.GetHostEntry(Dns.GetHostName());
        private IPAddress ipAddr;
        private IPEndPoint localEndPoint;

        public static DataToSend packet; // Will contain data of the vessel
        //called on startup of the plugin by Unity
        void Awake()
        {
            //ipAddr = ipHost.AddressList[0];
            //// read XML configuration file
            //xmlConfig.Load("PluginEelooPAD.xml");
            //// TODO: read XML configuration file
            //localEndPoint = new IPEndPoint(ipAddr, 11111);
            //// init a socket to send data to
            //sender = new Socket(ipAddr.AddressFamily, SocketType.Stream, ProtocolType.Tcp);
            //try
            //{
            //    sender.Connect(localEndPoint);
            //    byte[] messageSent = Encoding.ASCII.GetBytes("INIT");
            //    int byteSent = sender.Send(messageSent);
            //}
            //catch (Exception e)
            //{
            //    print("KSPSerialIO: Error connecting to socket: " + e.ToString());
            //}
            print("KSPSerialIO: Loading settings and socket connected...");
            Debug.Log("ASJDKJASHAKHJKAJSHDKAJSHDKJASHDKJAHSDKJAHSDKJHAKDHUWHKQUWXKJSHXKJ");
        }

        public void SendStruct()
        {
            // send data to socket
            byte[] messageSent = StructureToByteArray(packet);
            int byteSent = sender.Send(messageSent);
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