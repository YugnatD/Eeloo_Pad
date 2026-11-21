# Navball — ESP32-P4 (Waveshare ESP32-P4-WIFI6-Touch-LCD-3.4C)

KSP-style navball on the round MIPI-DSI panel: ball (sky / ground, horizon, parallels every 30°,
meridians every 30°), vessel symbol, and the prograde / retrograde, normal / anti-normal,
radial out / in, target / anti-target and maneuver markers. Data arrive as text lines over USB.

## Hardware notes
- ESP32-P4 **revision v3.2** → ESP-IDF **5.5.6** (`~/esp/v5.5.6/esp-idf`). IDF 5.5.1 does not support v3.x chips.
- The panel sold as "3.4C 800×800" behaves as **720×720** on this board: with the 800×800 BSP setting
  it drops ~80 columns in the middle of every line (image "cut in two"). `sdkconfig.defaults`
  therefore selects `CONFIG_BSP_LCD_TYPE_720_720_4_INCH`.
- In the Eeloo Pad, the module is wired to the USB hub (USB_DP / USB_DN): the PC talks to the
  P4's native USB port (USB-Serial-JTAG). For bench tests the USB-UART Type-C port (CH343,
  `/dev/ttyACM0`, also the console) accepts the same lines. Opening the CH343 port resets the board.

## Build / flash
```
. ~/esp/v5.5.6/esp-idf/export.sh
idf.py -p /dev/ttyACM0 flash monitor
```
If components.espressif.com is unreachable, the component manager hangs at configure time.
All components are already in `managed_components/`, so build offline with:
```
export IDF_COMPONENT_REGISTRY_URL=http://127.0.0.1:9 IDF_COMPONENT_STORAGE_URL=http://127.0.0.1:9
```

## Protocol (PC → navball), one line per item, degrees
```
A <heading> <pitch> <roll>     vessel attitude (heading N→E, pitch above horizon, roll + = right wing down)
P <heading> <pitch>            prograde        (retrograde drawn opposite)
N <heading> <pitch>            orbit normal    (anti-normal opposite)
R <heading> <pitch>            radial out      (radial in opposite)
T <heading> <pitch>            target          (anti-target opposite)
M <heading> <pitch>            maneuver node
P - | N - | R - | T - | M -    hide that marker
<h> <p> <r>                    three bare numbers = attitude (old RP2040 test format)
```
Directions are in the vessel's local north / east / up frame.

Binary frames from the KSP plugin (`plugin/KSP1_Serial`), mixed freely with text lines:
```
0xEE 0x5A | type | len | payload | crc8 (poly 0x07 over type, len, payload)
type 0x01 NAVBALL, 97 bytes: u8 flags (bit0..4 prograde, normal, radial, target, maneuver;
bits 5-6 speed mode), float3 right, top, nose, prograde, normal, radialOut, target, maneuver
(unit vectors, x = east, y = up, z = north; the vessel axes are used directly as the matrix)
```
The log line shows `rx frames` / `crc err` counters.
No line for 2 s → built-in demo. Simulator: `tools/navball_sim.py --port /dev/ttyACM0 --log`.

## Performance (720×720)
- Each core renders 16-row strips in internal SRAM, a DMA copies them to the PSRAM frame buffer.
- Inner loop on the P4 SIMD unit (`main/navball_simd.S`, 8 pixels at a time), checked at start-up
  against its C reference (`SIMD self-test passed` in the log). Scalar fallback: `NAVBALL_SIMD 0`.
- 17 ms per frame with the markers (≈ 58 fps max); capped at `TARGET_FPS 30` → ~50 % CPU free,
  0 % when the attitude does not change (no render). Triple buffering, swap at the end of a refresh.
- The log prints fps, render time and load every 2 s.

## Files
- `main/main.c` — display, buffers, strips + DMA, frame pacing, serial RX (UART0 + USB-Serial-JTAG), protocol, demo
- `main/navball.c` — ball renderer (fixed-point, attitude matrix, tables), scalar + SIMD row paths
- `main/navball_simd.S` — SIMD kernel
- `main/hud.c` — marker sprites (procedural, anti-aliased) and their blending
- `tools/navball_sim.py` — orbital scene simulator speaking the protocol
