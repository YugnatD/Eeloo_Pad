import os
import sys
import time
import socket


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
        msg = s.recv(1024).decode()
        if msg != '':
            print("Message recu: ", msg)
    # close the connection
    s.close() 
