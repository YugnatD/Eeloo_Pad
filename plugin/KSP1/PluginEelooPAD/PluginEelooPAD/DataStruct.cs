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
    // template for data to send
    [StructLayout(LayoutKind.Sequential, Pack = 1)]
    public struct DataToSend
    {
        public byte id;
        public float AP;
        public byte M1;
        public byte M2;
        public byte M3;
    }

}