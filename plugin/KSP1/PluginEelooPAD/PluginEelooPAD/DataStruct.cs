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
        public float PE;
        public float G;
        public int TAp;
        public int TPe;
        public int period;
        public float RAlt;
        public float Alt;
        public float Vsurf;
        public float VOrbit;
        public float deltaTime;
        public UInt32 MNTime;
        public float MNDeltaV;
        public float Pitch; 
        public float Roll;
        public float Heading;
        public UInt16 ActionGroups;  //46  status bit order:SAS, RCS, Light, Gear, Brakes, Abort, Custom01 - 10 
        public float TargetDist;     //53  Distance to targeted vessel (m)
        public float TargetV;        //54  Target vessel relative velocity (m/s)
        public byte NavballSASMode;  //55  Combined byte for navball target mode and SAS mode
                                     // First four bits indicate AutoPilot mode:
                                     // 0 SAS is off  //1 = Regular Stability Assist //2 = Prograde
                                     // 3 = RetroGrade //4 = Normal //5 = Antinormal //6 = Radial In
                                     // 7 = Radial Out //8 = Target //9 = Anti-Target //10 = Maneuver node
                                     // Last 4 bits set navball mode. (0=ignore,1=ORBIT,2=SURFACE,3=TARGET)
    }

    [StructLayout(LayoutKind.Sequential, Pack = 1)]
    public struct VesselControls
    {
        public Boolean SAS;
        public Boolean RCS;
        public Boolean Lights;
        public Boolean Gear;
        public Boolean Brakes;
        public Boolean Stage;
        public int Mode;
        public int SASMode;
        public int SpeedMode;
        public float Throttle;
        public float Pitch;
        public float Roll;
        public float Yaw;
        public float TX;
        public float TY;
        public float TZ;
        // public Boolean[] ControlGroup;
    };

}