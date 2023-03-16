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
            print(received.AP)
        else:
            print("Connection was closed by server")
            break

def threadSender(s):
    vessel = eps.EelooPadVessel()
    vessel.SAS = True
    while True:
        # wait for user input
        msg = input("Press Enter")
        # send message to the server
        print("Sending message")
        s.send(vessel.pack())
        vessel.SAS = not vessel.SAS


if __name__ == '__main__':
    # listen for message on port 11111
    # Create a socket object
    s = socket.socket()        
    
    # Define the port on which you want to connect
    port = 11111               
    
    # connect to the server on local computer
    s.connect(('127.0.0.1', port))

    # start a thread to listen for message
    t1 = threading.Thread(target=threadListener, args=(s,))
    t2 = threading.Thread(target=threadSender, args=(s,))
    t1.start()
    t2.start()

    t1.join()
    # should kill the thread 2
    t2.join()
    # close the connection
    s.close() 
