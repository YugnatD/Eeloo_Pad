/****************************************************************************************************
*** Author : Tanguy Dietrich / Kirill Goundiaev
*** Name : InFlight.cs
*** Description : This will manage the data to send to the socket when in fligh mode
***               And update the vessel state when in flight mode
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
using KSP.IO;
using KSP.UI.Screens;

namespace PluginEelooPAD
{
    [KSPAddon(KSPAddon.Startup.Flight, false)]
    public class UpdateDataKSP : MonoBehaviour
    {
        private static System.Timers.Timer aTimer;

        /****************************************************************************************************
        *** Name : Awake()
        *** Description : Called when starting a flight, Create a timer 
        *** Input : None
        *** Return : None
        *****************************************************************************************************/
        void Awake()
        {
            //init a timer to send data to the socket
            Debug.Log("[EelooPad] Awake Flight OK");
            //Debug.Log("[EelooPad] " + FlightGlobals.ActiveVessel.orbit.ApA.ToString());
            aTimer = new System.Timers.Timer(EelooCom.refreshRate);
        }

        /****************************************************************************************************
        *** Name : Start()
        *** Description : Called when starting a flight, init and start the timer (called after Awake)
        *** Input : None
        *** Return : None
        *****************************************************************************************************/
        void Start() //Called when starting a flight (After Awake)
        {
            Debug.Log("[EelooPad] Start Flight Method Called");
            aTimer.Elapsed += timerUpdate;
            aTimer.AutoReset = true;
            aTimer.Enabled = true;
            // FlightGlobals.ActiveVessel.OnFlyByWire += UpdateControls;
            FlightGlobals.ActiveVessel.OnPostAutopilotUpdate += UpdateControls;
        }

        /****************************************************************************************************
        *** Name : OnDestroy()
        *** Description : Called when leaving a flight, stop the timer and remove the event handler
                          We dont need to send data to the socket anymore
        *** Input : None
        *** Return : None
        *****************************************************************************************************/
        void OnDestroy()
        {
            Debug.Log("[EelooPad] OnDestroy Flight Method Called");
            aTimer.Stop();
            FlightGlobals.ActiveVessel.OnPostAutopilotUpdate -= UpdateControls;
        }

        /****************************************************************************************************
        *** Name : Update()
        *** Description : Called every frame, check if the stage button is pressed and if so, stage the vessel
        *** Input : None
        *** Return : None
        *****************************************************************************************************/
        void Update()
        {
            // Debug.Log("[EelooPad] Update Flight Method Called");
            if(FlightGlobals.ActiveVessel.ActionGroups[KSPActionGroup.Stage])
            {
                StageManager.ActivateNextStage();
                FlightGlobals.ActiveVessel.ActionGroups.SetGroup(KSPActionGroup.Stage, false);
            }
        }

        /****************************************************************************************************
        *** Name : timerUpdate()
        *** Description : Callback function called when the timer is elapsed
        *** Input : System.Object source -> The source of the event
        ***         ElapsedEventArgs e -> The event arguments
        *** Return : None
        *****************************************************************************************************/
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

        /****************************************************************************************************
        *** Name : UpdateControls()
        *** Description : Callback function called when the vessel is updated
        *** Input : FlightCtrlState s -> the actual control state
        *** Return : None
        *****************************************************************************************************/
        private void UpdateControls(FlightCtrlState s)
        {
            s.CopyFrom(EelooCom.actualControlState);
        }
    }
}


