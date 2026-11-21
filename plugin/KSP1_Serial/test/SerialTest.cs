// Bench test without KSP: sends NAVBALL frames built with the plugin's Protocol.cs over serial
// (Mono's SerialPort, like the plugin) and prints the board's log.
//   mcs -out:SerialTest.exe SerialTest.cs ../src/Protocol.cs -r:System.dll && mono SerialTest.exe /dev/ttyACM0
using System;
using System.IO.Ports;
using System.Threading;
using EelooPadSerial;

class SerialTest
{
    static Vec3 Dir(double hdg, double pitch)          // local frame x = east, y = up, z = north
    {
        double h = hdg * Math.PI / 180, p = pitch * Math.PI / 180;
        return new Vec3((float)(Math.Sin(h) * Math.Cos(p)), (float)Math.Sin(p), (float)(Math.Cos(h) * Math.Cos(p)));
    }

    static void Main(string[] args)
    {
        SerialPort port = new SerialPort(args.Length > 0 ? args[0] : "/dev/ttyACM0", 115200) { ReadTimeout = 10 };
        port.Open();
        Thread.Sleep(2500);                              // the CH343 port resets the board on open
        DateTime t0 = DateTime.Now;
        string rx = "";
        int sent = 0;
        while ((DateTime.Now - t0).TotalSeconds < 9)
        {
            double t = (DateTime.Now - t0).TotalSeconds;
            double hdg = 90 + 30 * Math.Sin(0.5 * t), pitch = 10 * Math.Sin(0.3 * t);
            NavballPacket p = new NavballPacket();
            p.nose = Dir(hdg, pitch);
            p.right = Dir(hdg + 90, 0);                  // wings level
            Vec3 n = p.nose, r = p.right;                // top = right x nose (component formula)
            p.top = new Vec3(r.y * n.z - r.z * n.y, r.z * n.x - r.x * n.z, r.x * n.y - r.y * n.x);
            p.prograde = Dir(80, 5); p.normal = Dir(20, 10); p.radial = Dir(0, 80);
            p.target = Dir(115, -10); p.maneuver = Dir(62, 24);
            p.flags = (byte)(t > 4 ? 0x1F : 0x07);       // target + maneuver appear after 4 s
            byte[] f = Protocol.Frame(Protocol.TYPE_NAVBALL, p.Payload());
            port.Write(f, 0, f.Length);
            sent++;
            try { rx += port.ReadExisting(); } catch (Exception) { }
            int nl;
            while ((nl = rx.IndexOf('\n')) >= 0)
            {
                string line = rx.Substring(0, nl).TrimEnd('\r');
                rx = rx.Substring(nl + 1);
                if (line.Contains("fps")) Console.WriteLine(line);
            }
            Thread.Sleep(33);
        }
        Console.WriteLine("frames sent: " + sent + ", frame size " + (97 + 5) + " bytes");
        port.Close();
    }
}
