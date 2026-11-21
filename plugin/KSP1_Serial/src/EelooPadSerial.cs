/****************************************************************************************************
*** Author : Tanguy Dietrich
*** Name : EelooPadSerial.cs
*** Description : Eeloo Pad plugin, serial version (follows the structure of plugin/KSP1):
***   - EelooPadSerial (loaded once at game start, lives for the whole session): reads
***     GameData/EelooPadSerial/config.xml and opens the serial links to the boards,
***   - EelooFlight (in flight): on Unity's main thread, reads the vessel state at the configured
***     rate and hands a binary frame to each link. All KSP API calls stay on the main thread;
***     the links only move bytes (see SerialLink.cs).
*****************************************************************************************************/
using System;
using System.Xml;
using UnityEngine;

namespace EelooPadSerial
{
    [KSPAddon(KSPAddon.Startup.Instantly, true)]
    public class EelooPadSerial : MonoBehaviour
    {
        public static SerialLink Navball;       // null when disabled in the config
        public static float NavballRate = 30;   // frames per second

        void Awake()
        {
            DontDestroyOnLoad(this);
            string path = KSPUtil.ApplicationRootPath + "GameData/EelooPadSerial/config.xml";
            try
            {
                XmlDocument cfg = new XmlDocument();
                cfg.Load(path);
                XmlNode nb = cfg.SelectSingleNode("/config/navball");
                if (nb != null && Text(nb, "enabled", "true") == "true")
                {
                    NavballRate = float.Parse(Text(nb, "rate", "30"), System.Globalization.CultureInfo.InvariantCulture);
                    Navball = new SerialLink("navball", Text(nb, "port", "/dev/ttyACM0"),
                                             int.Parse(Text(nb, "baud", "115200")),
                                             Text(nb, "log", "false") == "true");
                    Navball.Start();
                }
                Debug.Log("[EelooPad] config loaded from " + path);
            }
            catch (Exception e)
            {
                Debug.Log("[EelooPad] cannot load " + path + ": " + e.Message);
            }
        }

        void OnDestroy()
        {
            if (Navball != null) Navball.Stop();
        }

        private static string Text(XmlNode n, string child, string def)
        {
            XmlNode c = n.SelectSingleNode(child);
            return c != null ? c.InnerText.Trim() : def;
        }
    }

    [KSPAddon(KSPAddon.Startup.Flight, false)]
    public class EelooFlight : MonoBehaviour
    {
        private float nextNavball;

        void Update()
        {
            if (EelooPadSerial.Navball == null || !EelooPadSerial.Navball.Connected) return;
            if (Time.unscaledTime < nextNavball) return;
            nextNavball = Time.unscaledTime + 1f / EelooPadSerial.NavballRate;
            Vessel v = FlightGlobals.ActiveVessel;
            if (v == null || v.mainBody == null || v.ReferenceTransform == null) return;
            NavballPacket p = NavballData.Read(v);
            EelooPadSerial.Navball.Send(Protocol.Frame(Protocol.TYPE_NAVBALL, p.Payload()));
        }
    }

    public static class NavballData
    {
        /* Vessel state for the navball, in the local frame x = east, y = up, z = north. */
        public static NavballPacket Read(Vessel v)
        {
            CelestialBody body = v.mainBody;
            Vector3d com = v.CoMD;
            Vector3d up = (com - body.position).normalized;
            Vector3d north = Vector3d.Exclude(up, (body.position + (Vector3d)body.transform.up * body.Radius) - com).normalized;
            // world -> local: LookRotation(north, up) has forward = north, up = up, right = east
            Quaternion toLocal = Quaternion.Inverse(Quaternion.LookRotation((Vector3)north, (Vector3)up));

            NavballPacket p = new NavballPacket();
            Transform t = v.ReferenceTransform;               // KSP: up = nose, forward = belly
            p.right = Local(toLocal, t.right);
            p.top = Local(toLocal, -t.forward);
            p.nose = Local(toLocal, t.up);

            // prograde follows the navball speed mode, like KSP's own navball
            ITargetable tgt = v.targetObject;
            int mode = 0;
            Vector3d vel = v.obt_velocity;
            switch (FlightGlobals.speedDisplayMode)
            {
                case FlightGlobals.SpeedDisplayModes.Surface:
                    mode = 1; vel = v.srf_velocity; break;
                case FlightGlobals.SpeedDisplayModes.Target:
                    mode = 2; if (tgt != null) vel = v.obt_velocity - tgt.GetObtVelocity(); break;
            }
            int flags = mode << 5;
            if (vel.sqrMagnitude > 1e-4) { p.prograde = Local(toLocal, vel.normalized); flags |= 1; }

            if (mode == 0)                                    // normal / radial: orbit mode only (as in KSP)
            {
                Vector3d ov = v.obt_velocity, r = com - body.position;
                Vector3d normal = Vector3d.Cross(ov, r).normalized;
                Vector3d radial = Vector3d.Cross(normal, ov).normalized;
                if (normal.sqrMagnitude > 0.5) { p.normal = Local(toLocal, normal); flags |= 2; }
                if (radial.sqrMagnitude > 0.5) { p.radial = Local(toLocal, radial); flags |= 4; }
            }

            if (tgt != null && tgt.GetTransform() != null)
            {
                Vector3d d = (Vector3d)tgt.GetTransform().position - com;
                if (d.sqrMagnitude > 1e-6) { p.target = Local(toLocal, d.normalized); flags |= 8; }
            }

            PatchedConicSolver pcs = v.patchedConicSolver;
            if (pcs != null && pcs.maneuverNodes.Count > 0)
            {
                Vector3d burn = pcs.maneuverNodes[0].GetBurnVector(v.orbit);
                if (burn.sqrMagnitude > 1e-6) { p.maneuver = Local(toLocal, burn.normalized); flags |= 16; }
            }
            p.flags = (byte)flags;
            return p;
        }

        private static Vec3 Local(Quaternion toLocal, Vector3d world)
        {
            Vector3 l = toLocal * (Vector3)world;
            return new Vec3(l.x, l.y, l.z);
        }
    }
}
