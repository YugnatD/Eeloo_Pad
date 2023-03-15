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
    [KSPAddon(KSPAddon.Startup.Flight, false)]
    public class UpdateDataKSP : MonoBehaviour
    {
        //public static Vessel ActiveVessel = new Vessel();
        //called on startup of the plugin by Unity
        void Awake()
        {
            //init a timer to send data to the socket
            print("EelooPad: AWAKE");
            Debug.Log("EelooPad AWAKE CONSOLE");
        }

        //called once per frame
        //void Update()
        //{
        //    EelooCom.packet.AP = (float)ActiveVessel.orbit.ApA;
        //}
    }
}


