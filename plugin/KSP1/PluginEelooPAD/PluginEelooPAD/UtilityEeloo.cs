using System;
using System.Net;
using System.Net.Sockets;
using System.Text;
using System.Collections.Generic;
using System.Linq;
using System.Threading;
using System.Xml;
using System.Timers;
using UnityEngine;
using System.Runtime.InteropServices;

// Just some random function
namespace PluginEelooPAD
{
    
    public class UtilityEeloo
    {
        public static VesselControls MessageToControls(byte[] message)
        {
            VesselControls controls = new VesselControls();
            controls = (VesselControls)ByteArrayToStructure(message, controls);
            return controls;
        }

        //THANKS KSPSERIALIO
        public static byte[] StructureToByteArray(object obj)
        {
            int len = Marshal.SizeOf(obj);
            byte[] arr = new byte[len];
            IntPtr ptr = Marshal.AllocHGlobal(len);
            Marshal.StructureToPtr(obj, ptr, true);
            Marshal.Copy(ptr, arr, 0, len);
            Marshal.FreeHGlobal(ptr);
            return arr;
        }

        public static object ByteArrayToStructure(byte[] bytearray, object obj)
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


