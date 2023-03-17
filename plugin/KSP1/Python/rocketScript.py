import os
import sys
import time
import socket
import struct
import threading
import VesselControler as vc


if __name__ == '__main__':
    vessel = vc.VesselControler("127.0.0.1", 11111)
    vessel.connect()

    # wait for user input
    msg = input("Press Enter")
    time.sleep(5)

    # activate RCS and SAS
    vessel.setRCS(True)
    time.sleep(0.1)
    vessel.setSAS(True)
    time.sleep(0.1)
    # put Throttle to 50%
    vessel.setThrottle(500.0)
    time.sleep(0.1)
    # launch the stage
    vessel.setStage(True)
    time.sleep(0.1)
    # wait until the alitude is 1000m
    print(vessel.info.Alt)
    while vessel.info.Alt < 1000:
        time.sleep(0.1)
    time.sleep(0.1)
    # get the rocket to crash
    vessel.setThrottle(0)
    time.sleep(0.1)
    # set the Pitch to 1
    vessel.setPitch(1)
    time.sleep(0.1)
    # set the Roll to 1
    vessel.setRoll(1)
    time.sleep(0.1)
    # set the Yaw to -1
    vessel.setYaw(-1)
    time.sleep(0.1)

