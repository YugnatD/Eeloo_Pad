import os
import sys
import time
import socket
import struct
import EelooPadStructure as eps

if __name__ == '__main__':
    # listen for message on port 11111
    # Create a socket object
    s = socket.socket()        
    
    # Define the port on which you want to connect
    port = 11111               
    
    # connect to the server on local computer
    s.connect(('127.0.0.1', port))
    
    # receive data from the server and decoding to get the string.
    while True:
        # if received data is not empty
        msg = s.recv(1024) # need to make it waiting for message
        if msg:
            print(type(msg))
            received = eps.EelooPadStructure(msg)
            print(received.AP)

    # close the connection
    s.close() 
