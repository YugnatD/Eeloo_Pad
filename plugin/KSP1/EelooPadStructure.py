import struct
import numpy as np

# f = float
# B = unsigned char (Byte)
# b = signed char (Byte)
# h = short (16-bit)
# H = unsigned short (16-bit)
# i = int (32-bit)
# I = unsigned int  (32-bit)
# l = long 
# L = unsigned long
# q = long long
# Q = unsigned long long
# ? = bool

class EelooPadStructure:
    def __init__(self, lstBytes):
        items = struct.unpack('=BfffiiifffffIffffHffB', lstBytes)
        self.id: np.uint8 = items[0] # B
        self.AP: np.float = items[1] # f
        self.PE: np.float = items[2] # f
        self.G: np.float = items[3] # f
        self.TAp: np.int32 = items[4] # i
        self.TPe: np.int32 = items[5] # i
        self.period: np.int32 = items[6] # i
        self.RAlt: np.float = items[7] # f
        self.Alt: np.float = items[8] # f
        self.Vsurf: np.float = items[9] # f
        self.VOrbit: np.float = items[10] # f
        self.deltaTime: np.float = items[11] # f
        self.MNTime: np.uint32 = items[12] # I
        self.MNDeltaV: np.float = items[13] # f
        self.Pitch: np.float = items[14] # f
        self.Roll: np.float = items[15] # f
        self.Heading: np.float = items[16] # f
        self.ActionGroup: np.uint16 = items[17] # H
        self.TargetDist: np.float = items[18] # f
        self.TargetV: np.float = items[19] # f
        self.NavballSASMode: np.uint8 = items[20] # B
    # def __str__(self):
    #     return "id: " + str(self.id) + " AP: " + str(self.AP) + " PE: " + str(self.PE) + " G: " + str(self.G) + " TAp: " + str(self.TAp) + " TPe: " + str(self.TPe) + " period: " + str(self.period) + " RAlt: " + str(self.RAlt) + " Alt: " + str(self.Alt) + " Vsurf: " + str(self.Vsurf) + " VOrbit: " + str(self.VOrbit) + " deltaTime: " + str(self.deltaTime) + " MNTime: " + str(self.MNTime) + " MNDeltaV: " + str(self.MNDeltaV) + " Pitch: " + str(self.Pitch) + " Roll: " + str(self.Roll) + " Heading: " + str(self.Heading) + " ActionGroup: " + str(self.ActionGroup) + " TargetDist: " + str(self.TargetDist) + " TargetV: " + str(self.TargetV) + " NavballSASMode: " + str(self.NavballSASMode)

class EelooPadVessel:
    def __init__(self):
        self.SAS: np.bool = False # ?
        self.RCS: np.bool = False # ?
        self.Lights: np.bool = False # ?
        self.Gear: np.bool = False # ?
        self.Brakes: np.bool = False # ?
        self.Stage: np.bool = False # ?
        self.Mode: np.int32 = 0 # i
        self.SASMode: np.int32 = 0 # i
        self.SpeedMode: np.int32 = 0 # i
        self.Throttle: np.float = 0.0 # f
        self.Pitch: np.float = 0.0 # f
        self.Roll: np.float = 0.0 # f
        self.Yaw: np.float = 0.0 # f
        self.TX: np.float = 0.0 # f
        self.TY: np.float = 0.0 # f
        self.TZ: np.float = 0.0 # f

    def castStruct(self):
        self.SAS = np.bool(self.SAS)
        self.RCS = np.bool(self.RCS)
        self.Lights = np.bool(self.Lights)
        self.Gear = np.bool(self.Gear)
        self.Brakes = np.bool(self.Brakes)
        self.Stage = np.bool(self.Stage)
        self.Mode = np.int32(self.Mode)
        self.SASMode = np.int32(self.SASMode)
        self.SpeedMode = np.int32(self.SpeedMode)
        self.Throttle = np.float(self.Throttle)
        self.Pitch = np.float(self.Pitch)
        self.Roll = np.float(self.Roll)
        self.Yaw = np.float(self.Yaw)
        self.TX = np.float(self.TX)
        self.TY = np.float(self.TY)
        self.TZ = np.float(self.TZ)

    def pack(self):
        self.castStruct() # just to be sure the data used are correct
        # print(self.SAS, self.RCS, self.Lights, self.Gear, self.Brakes, self.Stage, self.Mode, self.SASMode, self.SpeedMode, self.Throttle, self.Pitch, self.Roll, self.Yaw, self.TX, self.TY, self.TZ)
        lstBytes = struct.pack('=??????iiifffffff', self.SAS, self.RCS, self.Lights, self.Gear, self.Brakes, self.Stage, self.Mode, self.SASMode, self.SpeedMode, self.Throttle, self.Pitch, self.Roll, self.Yaw, self.TX, self.TY, self.TZ)
        # print(lstBytes[20])
        print(lstBytes)
        print(len(lstBytes))
        return lstBytes

