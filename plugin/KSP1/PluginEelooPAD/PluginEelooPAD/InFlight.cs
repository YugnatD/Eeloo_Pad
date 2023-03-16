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
        //Called once when the plugin is loaded, and before start
        void Awake()
        {
            //init a timer to send data to the socket
            Debug.Log("[EelooPad] Awake Flight OK");
            //Debug.Log("[EelooPad] " + FlightGlobals.ActiveVessel.orbit.ApA.ToString());
            aTimer = new System.Timers.Timer(EelooCom.refreshRate);
        }

        void Start() //Called when starting a flight (Afeter Awake)
        {
            Debug.Log("[EelooPad] Start Flight Method Called");
            aTimer.Elapsed += timerUpdate;
            aTimer.AutoReset = true;
            aTimer.Enabled = true;
        }

        void OnDestroy()
        {
            Debug.Log("[EelooPad] OnDestroy Flight Method Called");
            aTimer.Stop();
        }

        private static void timerUpdate(System.Object source, ElapsedEventArgs e)
        {
            if (FlightGlobals.ActiveVessel != null)
            {
                // Debug.Log("[EelooPad] " + FlightGlobals.ActiveVessel.orbit.ApA.ToString());
                EelooCom.packet.id = 0x01;//(UInt16) FlightGlobals.ActiveVessel.id.;
                EelooCom.packet.AP = (float)FlightGlobals.ActiveVessel.orbit.ApA;
                EelooCom.packet.PE = (float)FlightGlobals.ActiveVessel.orbit.PeA;
                EelooCom.packet.G = (float)FlightGlobals.ActiveVessel.geeForce;
                EelooCom.packet.TAp = (int)FlightGlobals.ActiveVessel.orbit.timeToAp;
                EelooCom.packet.TPe = (int)FlightGlobals.ActiveVessel.orbit.timeToPe;
                EelooCom.packet.period = (int)FlightGlobals.ActiveVessel.orbit.period;
                EelooCom.packet.RAlt = (float)FlightGlobals.ActiveVessel.terrainAltitude;
                EelooCom.packet.Alt = (float)FlightGlobals.ActiveVessel.altitude;
                EelooCom.packet.Vsurf = (float)FlightGlobals.ActiveVessel.srfSpeed;
                EelooCom.packet.VOrbit = (float)FlightGlobals.ActiveVessel.obt_speed;
                // EelooCom.packet.deltaTime = (float)FlightGlobals.ActiveVessel;
                EelooCom.SendVesselInfo();
            }
            else
            {
                Debug.Log("FlightGlobals.ActiveVessel null");
            }
        }
    }
}


