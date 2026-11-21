# EelooPadSerial — KSP 1.12 plugin, serial version

Same structure as `../KSP1` (a session-wide addon that reads the config and opens the links, a
flight addon that reads the vessel), but the plugin talks **directly to the Eeloo Pad boards over
USB serial** (no TCP, no program running next to the game).

What is sent today: the **navball** frame to the ESP32-P4 board (`navball/esp32p4`), 30 times per
second: vessel axes and the prograde / normal / radial / target / maneuver directions, in the
vessel's local east / up / north frame. Prograde follows the navball speed mode (orbit, surface,
target); normal and radial are sent in orbit mode only, like KSP's own navball.

## Build / install (Linux, Mono)
```
./build.sh            # -> GameData/EelooPadSerial/EelooPadSerial.dll
./build.sh install    # + copies GameData/EelooPadSerial into KSP (keeps an existing config.xml)
```
`KSPDIR=/path/to/KSP ./build.sh install` for another install. References come from
`KSP_Data/Managed` of the install.

## Config — `GameData/EelooPadSerial/config.xml`
```xml
<navball>
  <enabled>true</enabled>
  <port>/dev/ttyACM0</port>   <!-- COMx on Windows -->
  <baud>115200</baud>
  <rate>30</rate>
  <log>true</log>             <!-- copy the board's log (fps, load, frames received) into KSP.log -->
</navball>
```
The link reconnects by itself when the board is plugged in later. Close any serial monitor on the
port before starting KSP. Lines starting with `[EelooPad]` in `KSP.log` show the link state.

## Protocol
```
frame   = 0xEE 0x5A | type | len | payload | crc8      (CRC-8 poly 0x07 over type, len, payload)
NAVBALL = type 0x01, 97 bytes, little-endian:
          u8 flags (bit0..4 prograde, normal, radial, target, maneuver valid; bits 5-6 speed mode)
          float3 right, top, nose, prograde, normal, radialOut, target, maneuver
```
All threads only move bytes: the KSP API is used on Unity's main thread (`EelooFlight.Update`).

## Bench test without KSP
`test/SerialTest.cs` builds frames with the plugin's `Protocol.cs` and sends them to the board:
```
cd test && mcs -out:SerialTest.exe SerialTest.cs ../src/Protocol.cs -r:System.dll && mono SerialTest.exe /dev/ttyACM0
```
