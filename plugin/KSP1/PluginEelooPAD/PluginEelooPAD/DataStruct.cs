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

    [StructLayout(LayoutKind.Sequential, Pack = 1)]
    public struct VesselControls
    {
        public Boolean SAS;
        // public Boolean RCS;
        // public Boolean Lights;
        // public Boolean Gear;
        // public Boolean Brakes;
        // public Boolean Precision;
        // public Boolean Abort;
        // public Boolean Stage;
        // public int Mode;
        // public int SASMode;
        // public int SpeedMode;
        // public Boolean[] ControlGroup;
        // public float Pitch;
        // public float Roll;
        // public float Yaw;
        // public float TX;
        // public float TY;
        // public float TZ;
        // public float WheelSteer;
        // public float Throttle;
        // public float WheelThrottle;
    };

}