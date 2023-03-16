import struct
import numpy as np

class EelooPadStructure:
    def __init__(self, lstBytes):
        items = struct.unpack('<BfBBB', lstBytes)
        self.id: np.uint8 = items[0]
        self.AP: float = items[1]
        self.M1: np.uint8 = items[2]
        self.M2: np.uint8 = items[3]
        self.M3: np.uint8 = items[4]

class EelooPadVessel:
    def __init__(self):
        # items = struct.unpack('<BfBBB', lstBytes)
        self.SAS: np.bool = False
    def pack(self):
        return struct.pack('<?', self.SAS)

