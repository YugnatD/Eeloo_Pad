import os
import sys
import time
import socket
import struct
import EelooPadStructure as eps
import threading

def threadListener(s):
    # receive data from the server and decoding to get the string.
    while True:
        # if received data is not empty
        msg = s.recv(1024) # need to make it waiting for message
        if msg:
            received = eps.EelooPadStructure(msg)
            print("AP :"+ str(received.AP) + " PE :" + str(received.PE))

        else:
            print("Connection was closed by server")
            break


if __name__ == '__main__':
    vessel = eps.EelooPadVessel()
    # listen for message on port 11111
    # Create a socket object
    s = socket.socket()        
    
    # Define the port on which you want to connect
    port = 11111               
    
    # connect to the server on local computer
    s.connect(('127.0.0.1', port))

    # start a thread to listen for message
    t1 = threading.Thread(target=threadListener, args=(s,))
    t1.start()

    #wait for user input to start the program
    msg = input("Press Enter")
    print("Setting up the vessel")
    vessel.SAS = True
    vessel.RCS = True
    vessel.Throttle = 1
    vessel.Stage = False
    print("Waiting 5 seconds")
    time.sleep(5)
    vessel.Stage = True
    s.send(vessel.pack())
    print("waiting 5 seconds")
    time.sleep(5)
    vessel.Stage = False
    #set Pitch to 1
    vessel.Pitch = 1
    s.send(vessel.pack())

    t1.join()
    # close the connection
    s.close() 
