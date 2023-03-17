/****************************************************************************************************
*** Author : Tanguy Dietrich / Kirill Goundiaev
*** Name : DataStruct.cs
*** Description : Containt some useful struct to send data and receive data
*****************************************************************************************************/
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
        /****************************************************************************************************
        *** Name : MessageToControls()
        *** Description : Convert a byte array to a VesselControls struct
        *** Input : byte[] message -> the message to convert
        *** Return : VesselControls -> the structure converted
        *****************************************************************************************************/
        public static VesselControls MessageToControls(byte[] message)
        {
            // string s = "";
            // foreach (byte b in message)
            // {
            //     s += b.ToString("X2") + " ";
            // }
            // Debug.Log("[EelooPad] " + s);
            VesselControls controls = new VesselControls();
            // Debug.Log("[EelooPad] " + Marshal.SizeOf(controls));
            controls = (VesselControls)ByteArrayToStructure(message, controls);
            return controls;
        }

        /****************************************************************************************************
        *** Name : StructureToByteArray()
        *** Description : Convert an object to a byte array
        *** Input : object obj -> the object to convert
        *** Return : byte[] -> the byte array converted
        *****************************************************************************************************/
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

        /****************************************************************************************************
        *** Name : ByteArrayToStructure()
        *** Description : Convert a byte array to an object
        *** Input : byte[] bytearray -> the byte array to convert
        *** Return : object -> the object converted
        *****************************************************************************************************/
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


