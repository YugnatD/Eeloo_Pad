/****************************************************************************************************
*** Author : Tanguy Dietrich
*** Name : Protocol.cs
*** Description : Binary frames sent to the Eeloo Pad boards over USB serial.
***
***   frame = 0xEE 0x5A | type (1 B) | len (1 B) | payload (len B) | crc8 (1 B)
***   crc8  = CRC-8, polynomial 0x07, init 0x00, over type, len and payload
***   all multi-byte values are little-endian, floats are IEEE-754 single precision
***
***   type 0x01 = NAVBALL (97 bytes), see NavballPacket
*****************************************************************************************************/
using System.IO;

namespace EelooPadSerial
{
    public static class Protocol
    {
        public const byte SYNC0 = 0xEE, SYNC1 = 0x5A;
        public const byte TYPE_NAVBALL = 0x01;

        public static byte Crc8(byte[] data, int offset, int count)
        {
            byte crc = 0;
            for (int i = offset; i < offset + count; i++)
            {
                crc ^= data[i];
                for (int b = 0; b < 8; b++)
                    crc = (byte)((crc & 0x80) != 0 ? (crc << 1) ^ 0x07 : crc << 1);
            }
            return crc;
        }

        public static byte[] Frame(byte type, byte[] payload)
        {
            byte[] f = new byte[payload.Length + 5];
            f[0] = SYNC0; f[1] = SYNC1; f[2] = type; f[3] = (byte)payload.Length;
            System.Array.Copy(payload, 0, f, 4, payload.Length);
            f[f.Length - 1] = Crc8(f, 2, payload.Length + 2);
            return f;
        }
    }

    /* NAVBALL payload (97 bytes). All vectors are unit vectors in the vessel's local frame
     * x = east, y = up, z = north (the navball frame of the ESP32-P4 firmware):
     *   u8      flags   bit0 prograde, bit1 normal, bit2 radial, bit3 target, bit4 maneuver valid,
     *                   bits 5-6 navball speed mode (0 orbit, 1 surface, 2 target)
     *   float3  right, top, nose      vessel axes (right wing, top of the cockpit, nose)
     *   float3  prograde, normal, radialOut, target, maneuver
     * Retrograde, anti-normal, radial in and anti-target are drawn opposite by the board. */
    public struct NavballPacket
    {
        public byte flags;
        public Vec3 right, top, nose, prograde, normal, radial, target, maneuver;

        public byte[] Payload()
        {
            MemoryStream ms = new MemoryStream(97);
            BinaryWriter w = new BinaryWriter(ms);
            w.Write(flags);
            foreach (Vec3 v in new[] { right, top, nose, prograde, normal, radial, target, maneuver })
            {
                w.Write(v.x); w.Write(v.y); w.Write(v.z);
            }
            w.Flush();
            return ms.ToArray();
        }
    }

    public struct Vec3
    {
        public float x, y, z;
        public Vec3(float x, float y, float z) { this.x = x; this.y = y; this.z = z; }
    }
}
