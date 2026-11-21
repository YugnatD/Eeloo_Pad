import os
import sys
import time
import serial


# the pico wait scanf("%hu %hu %hu\n", &yaw, &pitch, &roll);
# send every 20ms yaw pitch roll as uint16_t in degrees (0..359)
# over the serial port
# e.g. "123 45 67\n"

# open the serial port
SERIAL_PORT = '/dev/ttyACM0'  # change
BAUDRATE = 115200

if __name__ == '__main__':
    ser = serial.Serial(SERIAL_PORT, BAUDRATE, timeout=1)
    time.sleep(2)  # wait for the serial port to be ready

    yaw = 0
    pitch = 0
    roll = 0

    try:
        while True:
            line = f"{yaw} {pitch} {roll}\n"
            ser.write(line.encode('utf-8'))
            yaw = (yaw + 4) % 360
            pitch = (pitch + 2) % 360
            roll = (roll + 5) % 360
            time.sleep(0.02)  # 20ms
    except KeyboardInterrupt:
        ser.close()
        sys.exit(0)