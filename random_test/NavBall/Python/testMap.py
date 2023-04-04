#!/usr/bin/env python
# https://stackoverflow.com/questions/30265375/how-to-draw-orthographic-projection-from-equirectangular-projection

import math
import numpy as np
import scipy
import scipy.misc
import scipy.ndimage.interpolation
import subprocess
import imageio

def generateImage(pitch = 0, roll = 0, heading=0, size=256, textureFile = "NavBall_Texture.png"):
    src = imageio.imread(textureFile)

    # convert pitch and roll to radians
    pitch = pitch * math.pi / 180
    roll = roll * math.pi / 180
    # heading = heading * math.pi / 180

    # Image pixel co-ordinates
    px=np.arange(-1.0,1.0,2.0/size)+1.0/size
    py=np.arange(-1.0,1.0,2.0/size)+1.0/size
    hx,hy=scipy.meshgrid(px,py)

    # Compute z of sphere hit position, if pixel's ray hits
    r2=hx*hx+hy*hy
    hit=(r2<=1.0)
    hz=np.where(
        hit,
        -np.sqrt(1.0-np.where(hit,r2,0.0)),
        np.NaN
        )

    # Some spin and tilt to make things interesting
    spin=roll #2.0*np.pi*(frame+0.5)/frames
    cs=math.cos(spin)
    ss=math.sin(spin)
    ms=np.array([[cs,0.0,ss],[0.0,1.0,0.0],[-ss,0.0,cs]])

    tilt= pitch #0.125*np.pi*math.sin(2.0*spin)
    ct=math.cos(tilt)
    st=math.sin(tilt)
    mt=np.array([[1.0,0.0,0.0],[0.0,ct,st],[0.0,-st,ct]])

    # Rotate the hit points
    xyz=np.dstack([hx,hy,hz]) 
    xyz=np.tensordot(xyz,mt,axes=([2],[1]))
    xyz=np.tensordot(xyz,ms,axes=([2],[1]))
    x=xyz[:,:,0]
    y=xyz[:,:,1]
    z=xyz[:,:,2]

    # Compute map position of hit
    latitude =np.where(hit,(0.5+np.arcsin(y)/np.pi)*src.shape[0],0.0)
    longitude=np.where(hit,(1.0+np.arctan2(z,x)/np.pi)*0.5*src.shape[1],0.0)
    latlong=np.array([latitude,longitude])

    # Resample, and zap non-hit pixels
    dst=np.zeros((size,size,3))
    for channel in [0,1,2]:
        dst[:,:,channel]=np.where(
            hit,
            scipy.ndimage.interpolation.map_coordinates(
                src[:,:,channel],
                latlong,
                order=1
                ),
            0.0
            )
    # make a rotation of the image to match the heading
    # could propably be done in a better way when generating the latlong array
    dst = scipy.ndimage.interpolation.rotate(dst, heading, reshape=False)
    # set a red square at the center
    CROSS_SIZE = 50
    dst[size//2-int(CROSS_SIZE/2):size//2+int(CROSS_SIZE/2),size//2-int(CROSS_SIZE/2):size//2+int(CROSS_SIZE/2),0] = 255
    return dst

if __name__ == "__main__":
    dst = generateImage(pitch = 45, roll = 90, heading = 45, size = 1024, textureFile = 'NavBall_Texture.png')
    imageio.imwrite('result.png',dst)