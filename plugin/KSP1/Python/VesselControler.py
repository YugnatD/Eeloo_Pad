import os
import sys
import time
import threading
import socket
import struct
import numpy as np

class VesselData:
  def __init__(self, lstBytes):
    if lstBytes is None:
      self.id: np.uint8 = 0 # B
      self.AP: np.float = 0.0
      self.PE: np.float = 0.0
      self.G: np.float = 0.0
      self.TAp: np.int32 = 0
      self.TPe: np.int32 = 0
      self.period: np.int32 = 0
      self.RAlt: np.float = 0.0
      self.Alt: np.float = 0.0
      self.Vsurf: np.float = 0.0
      self.VOrbit: np.float = 0.0
      self.deltaTime: np.float = 0.0
      self.MNTime: np.uint32 = 0
      self.MNDeltaV: np.float = 0.0
      self.Pitch: np.float = 0.0
      self.Roll: np.float = 0.0
      self.Heading: np.float = 0.0
      self.ActionGroup: np.uint16 = 0
      self.TargetDist: np.float = 0.0
      self.TargetV: np.float = 0.0
      self.NavballSASMode: np.uint8 = 0
    else:
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

class VesselControler:
  def __init__(self, ip, port):
    # Attribut to control the communication
    self.ip = ip
    self.port = port
    self.s = socket.socket()
    self.timeLastCommand = time.time()
    self.MINWAITCMD = 0.03 # 30ms
    # Attribut to control the vessel
    self.__SAS: np.bool = False # ?
    self.__RCS: np.bool = False # ?
    self.__Lights: np.bool = False # ?
    self.__Gear: np.bool = False # ?
    self.__Brakes: np.bool = False # ?
    self.__Stage: np.bool = False # ?
    self.__Mode: np.int32 = 0 # i
    self.__SASMode: np.int32 = 0 # i
    self.__SpeedMode: np.int32 = 0 # i
    self.__Throttle: np.float = 0.0 # f
    self.__Pitch: np.float = 0.0 # f
    self.__Roll: np.float = 0.0 # f
    self.__Yaw: np.float = 0.0 # f
    self.__TX: np.float = 0.0 # f
    self.__TY: np.float = 0.0 # f
    self.__TZ: np.float = 0.0 # f
    # Attribut to get the vessel data
    self.info: VesselData = VesselData(None)
    
  def __threadListener(self):
    # receive data from the server and decoding to get the string.
    while True:
        # if received data is not empty
        msg = self.s.recv(256) # need to make it waiting for message
        if msg:
           # TODO: BE CAREFUL OF CONCURENCY
           self.info = VesselData(msg)
        else:
            print("Connection was closed by server")
            break
    
  def connect(self):
    # init the internal class
    self.s.connect((self.ip, self.port))
    self.threadListener = threading.Thread(target=self.__threadListener, args=())
    self.threadListener.start()
  
  def disconnect(self):
    self.s.close()
    self.threadListener.join()

  def __castStruct(self):
    self.__SAS = np.bool(self.__SAS)
    self.__RCS = np.bool(self.__RCS)
    self.__Lights = np.bool(self.__Lights)
    self.__Gear = np.bool(self.__Gear)
    self.__Brakes = np.bool(self.__Brakes)
    self.__Stage = np.bool(self.__Stage)
    self.__Mode = np.int32(self.__Mode)
    self.__SASMode = np.int32(self.__SASMode)
    self.__SpeedMode = np.int32(self.__SpeedMode)
    self.__Throttle = np.float(self.__Throttle)
    self.__Pitch = np.float(self.__Pitch)
    self.__Roll = np.float(self.__Roll)
    self.__Yaw = np.float(self.__Yaw)
    self.__TX = np.float(self.__TX)
    self.__TY = np.float(self.__TY)
    self.__TZ = np.float(self.__TZ)

  def __pack(self):
    # self.__castStruct() # just to be sure the data used are correct
    lstBytes = struct.pack('=??????iiifffffff', self.__SAS, self.__RCS, self.__Lights, self.__Gear, self.__Brakes, self.__Stage, self.__Mode, self.__SASMode, self.__SpeedMode, self.__Throttle, self.__Pitch, self.__Roll, self.__Yaw, self.__TX, self.__TY, self.__TZ)
    return lstBytes
  
  def __waitLastCommand(self):
    time.sleep(max(0, self.MINWAITCMD - (time.time() - self.timeLastCommand)))
  
  # setter for the vessel control
  def setSAS(self, value: np.bool):
    self.__waitLastCommand()
    self.__SAS = np.bool(value)
    self.s.send(self.__pack())
    self.timeLastCommand = time.time()
  
  def setRCS(self, value: np.bool):
    self.__waitLastCommand()
    self.__RCS = np.bool(value)
    self.s.send(self.__pack())
    self.timeLastCommand = time.time()
  
  def setLights(self, value: np.bool):
    self.__waitLastCommand()
    self.__Lights = np.bool(value)
    self.s.send(self.__pack())
    self.timeLastCommand = time.time()
  
  def setGear(self, value: np.bool):
    self.__waitLastCommand()
    self.__Gear = np.bool(value)
    self.s.send(self.__pack())
    self.timeLastCommand = time.time()
  
  def setBrakes(self, value: np.bool):
    self.__waitLastCommand()
    self.__Brakes = np.bool(value)
    self.s.send(self.__pack())
    self.timeLastCommand = time.time()

  def setStage(self, value: np.bool):
    self.__waitLastCommand()
    self.__Stage = np.bool(value)
    self.s.send(self.__pack())
    self.__Stage = np.bool(False)
    self.timeLastCommand = time.time()
  
  def setMode(self, value: np.int32):
    self.__waitLastCommand()
    self.__Mode = np.int32(value)
    self.s.send(self.__pack())
    self.timeLastCommand = time.time()
  
  def setSASMode(self, value: np.int32):
    self.__waitLastCommand()
    self.__SASMode = np.int32(value)
    self.s.send(self.__pack())
    self.timeLastCommand = time.time()
  
  def setSpeedMode(self, value: np.int32):
    self.__waitLastCommand()
    self.__SpeedMode = np.int32(value)
    self.s.send(self.__pack())
    self.timeLastCommand = time.time()
  
  def setThrottle(self, value: np.float):
    self.__waitLastCommand()
    self.__Throttle = np.float(value)
    self.s.send(self.__pack())
    self.timeLastCommand = time.time()
  
  def setPitch(self, value: np.float):
    self.__waitLastCommand()
    self.__Pitch = np.float(value)
    self.s.send(self.__pack())
    self.timeLastCommand = time.time()
  
  def setRoll(self, value: np.float):
    self.__waitLastCommand()
    self.__Roll = np.float(value)
    self.s.send(self.__pack())
  
  def setYaw(self, value: np.float):
    self.__waitLastCommand()
    self.__Yaw = np.float(value)
    self.s.send(self.__pack())
    self.timeLastCommand = time.time()
  
  def setTX(self, value: np.float):
    self.__waitLastCommand()
    self.__TX = np.float(value)
    self.s.send(self.__pack())
    self.timeLastCommand = time.time()
  
  def setTY(self, value: np.float):
    self.__waitLastCommand()
    self.__TY = np.float(value)
    self.s.send(self.__pack())
    self.timeLastCommand = time.time()
  
  def setTZ(self, value: np.float):
    self.__waitLastCommand()
    self.__TZ = np.float(value)
    self.s.send(self.__pack())
    self.timeLastCommand = time.time()
  