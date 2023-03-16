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
# < = little endian
# > = big endian

class EelooPadStructure:
    def __init__(self, lstBytes):
        items = struct.unpack('<BfffiiifffffIIfffHffB', lstBytes)
        self.id: np.uint8 = items[0] # B
        self.AP: float = items[1] # f
        self.PE: float = items[2] # f
        self.G: float = items[3] # f
        self.TAp: np.int32 = items[4] # i
        self.TPe: np.int32 = items[5] # i
        self.period: np.int32 = items[6] # i
        self.RAlt: float = items[7] # f
        self.Alt: float = items[8] # f
        self.Vsurf: float = items[9] # f
        self.VOrbit: float = items[10] # f
        self.deltaTime: float = items[11] # f
        self.MNTime: np.uint32 = items[12] # I
        self.MNDeltaV: float = items[13] # I
        self.Pitch: float = items[14] # f
        self.Roll: float = items[15] # f
        self.Heading: float = items[16] # f
        self.ActionGroup: np.uint16 = items[17] # H
        self.TargetDist: float = items[18] # f
        self.TargetV: float = items[19] # f
        self.NavballSASMode: np.uint8 = items[20] # B
    # def __str__(self):
    #     return "id: " + str(self.id) + " AP: " + str(self.AP) + " PE: " + str(self.PE) + " G: " + str(self.G) + " TAp: " + str(self.TAp) + " TPe: " + str(self.TPe) + " period: " + str(self.period) + " RAlt: " + str(self.RAlt) + " Alt: " + str(self.Alt) + " Vsurf: " + str(self.Vsurf) + " VOrbit: " + str(self.VOrbit) + " deltaTime: " + str(self.deltaTime) + " MNTime: " + str(self.MNTime) + " MNDeltaV: " + str(self.MNDeltaV) + " Pitch: " + str(self.Pitch) + " Roll: " + str(self.Roll) + " Heading: " + str(self.Heading) + " ActionGroup: " + str(self.ActionGroup) + " TargetDist: " + str(self.TargetDist) + " TargetV: " + str(self.TargetV) + " NavballSASMode: " + str(self.NavballSASMode)



class EelooPadVessel:
    def __init__(self):
        # items = struct.unpack('<BfBBB', lstBytes)
        self.SAS: np.bool = False
        self.RCS: np.bool = False
        self.Lights: np.bool = False
        self.Gear: np.bool = False
        self.Brakes: np.bool = False
        self.Stage: np.bool = False
        self.Mode: np.int32 = 0
        self.SASMode: np.int32 = 0
        self.SpeedMode: np.int32 = 0
        self.Throttle: float = 0.0
        self.Pitch: float = 0.0
        self.Roll: float = 0.0
        self.Yaw: float = 0.0
        self.TX: float = 0.0
        self.TY: float = 0.0
        self.TZ: float = 0.0
    def pack(self):
        return struct.pack('<?', self.SAS)

