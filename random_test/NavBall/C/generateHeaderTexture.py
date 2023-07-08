# Simple program to convert a pgm file to a strcuture in C
# the structure is as follow :
# define TEXTURE_MAP_WIDTH 1024
# define TEXTURE_MAP_HEIGHT 512
# typedef struct _textureMap {
#   uint8_t data[TEXTURE_MAP_HEIGHT][TEXTURE_MAP_WIDTH][3];
# } textureMap_t;
import os
import sys
import time
import numpy as np
from pgm_reader import Reader


if __name__ == '__main__':
    # open the file NavBall_Texture.ppm
    f = 'NavBall_Texture.ppm'
    reader = Reader()
    image = reader.read_pgm(f)
    width = reader.width
    height = reader.height
    print('width: ', width)
    print('height: ', height)
    
