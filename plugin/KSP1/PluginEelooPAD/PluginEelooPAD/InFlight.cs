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

namespace PluginEelooPAD
{
    [KSPAddon(KSPAddon.Startup.Flight, false)]
    public class UpdateDataKSP : MonoBehaviour
    {
        private static System.Timers.Timer aTimer;
        //public static Vessel ActiveVessel = new Vessel();
        //called on startup of the plugin by Unity
        void Awake()
        {
            //init a timer to send data to the socket
            Debug.Log("[EelooPad] Awake Flight OK");
            //Debug.Log("[EelooPad] " + FlightGlobals.ActiveVessel.orbit.ApA.ToString());
            aTimer = new System.Timers.Timer(500);
            aTimer.Elapsed += timerUpdate;
            aTimer.AutoReset = true;
            aTimer.Enabled = true;
        }

        private static void timerUpdate(System.Object source, ElapsedEventArgs e)
        {
            if (FlightGlobals.ActiveVessel != null)
            {
                Debug.Log("[EelooPad] " + FlightGlobals.ActiveVessel.orbit.ApA.ToString());
                EelooCom.packet.AP = (float)FlightGlobals.ActiveVessel.orbit.ApA;
                EelooCom.packet.id = 0x01;
                EelooCom.SendStruct();
            }
            else
            {
                Debug.Log("FlightGlobals.ActiveVessel null");
            }
        }
    }
}


