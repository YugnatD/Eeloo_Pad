/****************************************************************************************************
*** Author : Tanguy Dietrich
*** Name : SerialLink.cs
*** Description : One USB serial device (e.g. the navball). A background thread owns the port:
***               it (re)opens it when the board is plugged, writes the latest frame given by the
***               game (older unsent frames are dropped, the board always gets the freshest state)
***               and reads what the board prints (its log) so nothing piles up on the USB link.
***               No KSP API is used here: the game data are produced on Unity's main thread.
*****************************************************************************************************/
using System;
using System.IO.Ports;
using System.Text;
using System.Threading;
using UnityEngine;

namespace EelooPadSerial
{
    public class SerialLink
    {
        private readonly string name, portName;
        private readonly int baud;
        private readonly bool logBoard;
        private SerialPort port;
        private Thread thread;
        private volatile bool running;
        private byte[] pending;                 // latest frame to send (guarded by 'sync')
        private readonly object sync = new object();
        private readonly AutoResetEvent wake = new AutoResetEvent(false);
        private readonly StringBuilder rxLine = new StringBuilder();
        private bool drainErrorLogged;

        public bool Connected { get { SerialPort p = port; return p != null && p.IsOpen; } }

        public SerialLink(string name, string portName, int baud, bool logBoard)
        {
            this.name = name; this.portName = portName; this.baud = baud; this.logBoard = logBoard;
        }

        public void Start()
        {
            running = true;
            thread = new Thread(Run) { IsBackground = true, Name = "EelooPad " + name };
            thread.Start();
        }

        public void Stop()
        {
            running = false;
            wake.Set();
            if (thread != null) thread.Join(1000);
            Close();
        }

        /* called from the game: keep only the most recent frame */
        public void Send(byte[] frame)
        {
            lock (sync) pending = frame;
            wake.Set();
        }

        private void Run()
        {
            DateTime nextTry = DateTime.MinValue;
            while (running)
            {
                if (!Connected)
                {
                    if (DateTime.Now < nextTry) { wake.WaitOne(200); continue; }
                    nextTry = DateTime.Now.AddSeconds(2);
                    Open();
                    continue;
                }
                wake.WaitOne(50);
                byte[] f;
                lock (sync) { f = pending; pending = null; }
                try
                {
                    if (f != null) port.Write(f, 0, f.Length);
                }
                catch (Exception e)
                {
                    /* only a failed write means the board is gone (reopening the port resets
                     * a board behind a USB-UART bridge, so do not close it for anything else) */
                    Debug.Log("[EelooPad] " + name + " (" + portName + ") lost: " + e.GetType().Name + " " + e.Message);
                    Close();
                    continue;
                }
                try { Drain(); }
                catch (Exception e)
                {
                    if (!drainErrorLogged) Debug.Log("[EelooPad] " + name + ": cannot read the board log: " + e.GetType().Name + " " + e.Message);
                    drainErrorLogged = true;
                }
            }
        }

        private void Open()
        {
            try
            {
                port = new SerialPort(portName, baud, Parity.None, 8, StopBits.One)
                {
                    Handshake = Handshake.None,
                    ReadTimeout = 10,
                    WriteTimeout = 500,
                };
                port.Open();
                Debug.Log("[EelooPad] " + name + " connected on " + portName);
            }
            catch (Exception e)
            {
                Debug.Log("[EelooPad] " + name + ": cannot open " + portName + " (" + e.Message + "), retrying");
                port = null;
            }
        }

        private void Close()
        {
            try { if (port != null) port.Close(); } catch (Exception) { }
            port = null;
        }

        /* read what the board prints; forward its log lines to KSP.log when asked */
        private void Drain()
        {
            int n = port.BytesToRead;
            if (n <= 0) return;
            byte[] b = new byte[n];
            n = port.Read(b, 0, n);
            if (!logBoard) return;
            for (int i = 0; i < n; i++)
            {
                char c = (char)b[i];
                if (c == '\n')
                {
                    if (rxLine.Length > 0) Debug.Log("[EelooPad] " + name + " > " + rxLine.ToString().TrimEnd(new[] { '\r' }));
                    rxLine.Length = 0;
                }
                else if (rxLine.Length < 300 && c >= ' ') rxLine.Append(c);
            }
        }
    }
}
