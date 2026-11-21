#!/usr/bin/env python3
"""Navball data simulator: plays a small orbital scene and streams it to the ESP32-P4 navball
with the same text protocol the KSP bridge will use (see PROTOCOL in main/main.c):

    A <heading> <pitch> <roll>      attitude
    P|N|R|T|M <heading> <pitch>     prograde / normal / radial out / target / maneuver
    P|N|R|T|M -                     hide that marker

Scene: circular, inclined orbit around a body. Marker directions are computed in the local
north / east / up frame of the vessel, exactly like KSP mods report them. The vessel points
at prograde, then turns to the maneuver node, then to the target, in a loop. The maneuver node
appears and disappears, the target is hidden for the first seconds (tests the '-' lines).

usage: navball_sim.py [--port /dev/ttyACM0] [--rate 30] [--duration 0] [--log]
  --log   also print the navball's own log lines (fps, load) read from the same port
"""
import argparse
import math
import sys
import time

import serial


def norm(v):
    n = math.sqrt(sum(c * c for c in v))
    return [c / n for c in v]


def cross(a, b):
    return [a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]]


def dot(a, b):
    return sum(x * y for x, y in zip(a, b))


def lerp_dir(a, b, t):
    t = max(0.0, min(1.0, t))
    t = t * t * (3 - 2 * t)                      # smooth start / stop
    return norm([x * (1 - t) + y * t for x, y in zip(a, b)])


def scene(t):
    """returns (attitude (h, p, r), {marker: (h, p) or None})"""
    R, inc = 1.0, math.radians(28)               # orbit radius, inclination
    w = 2 * math.pi / 120                        # one orbit every 2 minutes
    th = w * t
    # vessel position / velocity, orbit plane tilted around X by the inclination
    p = [R * math.cos(th), R * math.sin(th) * math.cos(inc), R * math.sin(th) * math.sin(inc)]
    v = norm([-math.sin(th), math.cos(th) * math.cos(inc), math.cos(th) * math.sin(inc)])
    # local frame: up, north (towards the body's +Z pole), east
    U = norm(p)
    Zw = [0, 0, 1]
    N = norm([z - dot(Zw, U) * u for z, u in zip(Zw, U)])
    E = cross(N, U)

    def hp(d):                                   # world direction -> heading, pitch (degrees)
        e, n, u = dot(d, E), dot(d, N), dot(d, U)
        return math.degrees(math.atan2(e, n)) % 360, math.degrees(math.asin(max(-1, min(1, u))))

    normal = norm(cross(p, v))
    radial = norm(cross(v, normal))
    # target: a station 3 degrees ahead on a slightly higher orbit
    th2 = th + math.radians(3)
    q = [1.02 * math.cos(th2), 1.02 * math.sin(th2) * math.cos(inc), 1.02 * math.sin(th2) * math.sin(inc)]
    target = norm([a - b for a, b in zip(q, p)])
    maneuver = norm([0.8 * a + 0.5 * b + 0.3 * c for a, b, c in zip(v, normal, radial)])

    # vessel nose: prograde -> maneuver -> target -> prograde (60 s cycle)
    c = t % 60
    if c < 20:
        nose = lerp_dir(target, v, (c - 0) / 6)
    elif c < 40:
        nose = lerp_dir(v, maneuver, (c - 20) / 6)
    else:
        nose = lerp_dir(maneuver, target, (c - 40) / 6)
    wob = [0.03 * math.sin(1.3 * t), 0.03 * math.sin(1.7 * t), 0.03 * math.sin(1.1 * t)]
    nose = norm([a + b for a, b in zip(nose, wob)])
    h, pch = hp(nose)
    roll = 18 * math.sin(0.25 * t)

    markers = {
        'P': hp(v),
        'N': hp(normal),
        'R': hp(radial),
        'T': hp(target) if t > 8 else None,
        'M': hp(maneuver) if (t % 45) < 35 else None,
    }
    return (h, pch, roll), markers


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('--port', default='/dev/ttyACM0')
    ap.add_argument('--rate', type=float, default=30)
    ap.add_argument('--duration', type=float, default=0, help='seconds, 0 = forever')
    ap.add_argument('--log', action='store_true', help="print the navball's log lines")
    a = ap.parse_args()

    s = serial.Serial()
    s.port, s.baudrate, s.timeout = a.port, 115200, 0
    s.dtr = s.rts = False                        # do not reset the board when opening the port
    s.open()
    t0 = time.time()
    rx = b''
    try:
        while True:
            t = time.time() - t0
            if a.duration and t > a.duration:
                break
            (h, p, r), mk = scene(t)
            lines = [f'A {h:.2f} {p:.2f} {r:.2f}']
            for k, d in mk.items():
                lines.append(f'{k} {d[0]:.2f} {d[1]:.2f}' if d else f'{k} -')
            s.write(('\n'.join(lines) + '\n').encode())
            rx += s.read(4096)
            while b'\n' in rx:
                line, rx = rx.split(b'\n', 1)
                if a.log and b'navball' in line:
                    print(line.decode('utf-8', 'replace').rstrip())
            time.sleep(max(0.0, 1 / a.rate - (time.time() - t0 - t)))
    except KeyboardInterrupt:
        pass
    s.close()


if __name__ == '__main__':
    sys.exit(main())
