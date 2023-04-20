#!/usr/bin/env python
# https://stackoverflow.com/questions/30265375/how-to-draw-orthographic-projection-from-equirectangular-projection

import math
import numpy as np
import scipy
import scipy.misc
import scipy.ndimage.interpolation
import subprocess
import imageio
import time

# def tensordot2(a, b):
#     a, b = np.asarray(a), np.asarray(b)
#     # newaxes_a = [0,1,2]
#     newshape_a = (a.shape[0]*a.shape[1],a.shape[2])
#     olda = [a.shape[0],a.shape[1]]
#     # newaxes_b = [1,0]
#     newshape_b = (b.shape[0],b.shape[0])
#     oldb = [b.shape[1]]
#     at = a.reshape(newshape_a) # 16,3
#     bt = b.T.reshape(newshape_b) # 3,3
#     res = np.zeros(newshape_a)
#     for i in range(newshape_a[0]):
#         for j in range(newshape_a[1]):
#             res[i,j] = at[i,0]*bt[0,j] + at[i,1]*bt[1,j] + at[i,2]*bt[2,j]
#     return res.reshape(olda + oldb)

# def tensordot2(a, b):
#     a, b = np.asarray(a), np.asarray(b)
#     newshape_a = (a.shape[0]*a.shape[1],a.shape[2])
#     olda = [a.shape[0],a.shape[1]]
#     newshape_b = (b.shape[0],b.shape[0])
#     oldb = [b.shape[1]]
#     at = a.reshape(newshape_a) # 16,3
#     bt = b.reshape(newshape_b) # 3,3
#     res = np.zeros(newshape_a)
#     for i in range(newshape_a[0]):
#         for j in range(newshape_a[1]):
#             res[i,j] = at[i,0]*bt[j,0] + at[i,1]*bt[j,1] + at[i,2]*bt[j,2]
#     return res.reshape(olda + oldb)

def tensordot2(a, b):
    a, b = np.asarray(a), np.asarray(b)
    res = np.zeros(a.shape)
    for i in range (a.shape[0]): # 4
        for j in range(a.shape[1]): # 4
            for k in range(a.shape[2]): # 3
                res[i,j,k] = a[i,j,0]*b[k,0] + a[i,j,1]*b[k,1] + a[i,j,2]*b[k,2]
    return res



def generateImage(pitch = 0, roll = 0, heading=0, size=256, textureFile = "NavBall_Texture.png"):
    src = imageio.imread(textureFile)

    # convert pitch and roll to radians
    pitch = pitch * math.pi / 180
    roll = roll * math.pi / 180
    # heading = heading * math.pi / 180

    # Image pixel co-ordinates
    px=np.arange(-1.0,1.0,2.0/size)+1.0/size
    py=np.arange(-1.0,1.0,2.0/size)+1.0/size
    # print(px)
    # print(py)

    hx,hy=scipy.meshgrid(px,py)
    # print(hx)
    # print(hy)

    # Compute z of sphere hit position, if pixel's ray hits
    r2=hx*hx+hy*hy
    hit=(r2<=1.0)
    hz=np.where(
        hit,
        -np.sqrt(1.0-np.where(hit,r2,0.0)),
        np.NaN
        )
    # print(hz)
    # print(hit)

    # Some spin and tilt to make things interesting
    spin=roll #2.0*np.pi*(frame+0.5)/frames
    cs=math.cos(spin)
    ss=math.sin(spin)
    ms=np.array([[cs,0.0,ss],[0.0,1.0,0.0],[-ss,0.0,cs]])
    # print(ms)

    tilt= pitch #0.125*np.pi*math.sin(2.0*spin)
    ct=math.cos(tilt)
    st=math.sin(tilt)
    mt=np.array([[1.0,0.0,0.0],[0.0,ct,st],[0.0,-st,ct]])
    # print(mt)

    # Rotate the hit points
    xyz=np.dstack([hx,hy,hz]) 
    # print(xyz)
    # xyz=np.tensordot(xyz,mt,axes=([2],[1]))
    xyz = tensordot2(xyz,mt)
    # print(xyz)
    # xyz=np.tensordot(xyz,ms,axes=([2],[1]))
    xyz = tensordot2(xyz,ms)
    # print(xyz)
    x=xyz[:,:,0]
    y=xyz[:,:,1]
    z=xyz[:,:,2]

    # Compute map position of hit
    print(src.shape[0])
    print(src.shape[1])
    latitude =np.where(hit,(0.5+np.arcsin(y)/np.pi)*src.shape[0],0.0)
    print("latitude")
    print(latitude)
    longitude=np.where(hit,(1.0+np.arctan2(z,x)/np.pi)*0.5*src.shape[1],0.0)
    print("longitude")
    print(longitude)
    latlong=np.array([latitude,longitude])

    # Resample, and zap non-hit pixels
    dst=np.zeros((size,size,3))
    # for coord in latlong:
    # for channel in [0,1,2]:
    #     # print(coord)
    #     # print("channel" + str(channel))
    #     # print("src[:,:,channel]" + str(src[:,:,channel][0]))
    #     interpolated = scipy.ndimage.interpolation.map_coordinates(src[:,:,channel],latlong,order=1)
    #     # print("latlong" + str(latlong[0]))
    #     # print("interpolated" + str(interpolated[0]))
    #     val = np.where(hit,interpolated,0.0)
    #     dst[:,:,channel]= val
    #     # print(latlong, val)
    #     # print the lat long and the corresponding pixel r,g,b interpolated values
    # print(src.shape)
    x = 315
    y = 125
    print("texture["+str(x)+","+str(y)+"] " + str(src[x,y,0]) + " " + str(src[x,y,1]) + " " + str(src[x,y,2]))
    for x in range(size):
        for y in range(size):
            if hit[x,y]:
                # print("x " + str(x) + " y " + str(y))
                # print("latitude["+str(x)+","+str(y)+"] " + str(latitude[x,y]) + " longitude["+str(x)+","+str(y)+"] " + str(longitude[x,y]))          
                r = src[int(latitude[x,y]),int(longitude[x,y]),0]
                g = src[int(latitude[x,y]),int(longitude[x,y]),1]
                b = src[int(latitude[x,y]),int(longitude[x,y]),2]
                # print(r,g,b)
                dst[x,y,0] = r
                dst[x,y,1] = g
                dst[x,y,2] = b

    # make a rotation of the image to match the heading
    # could propably be done in a better way when generating the latlong array
    # dst = scipy.ndimage.interpolation.rotate(dst, heading, reshape=False)
    # set a red square at the center
    # CROSS_SIZE = 50
    # dst[size//2-int(CROSS_SIZE/2):size//2+int(CROSS_SIZE/2),size//2-int(CROSS_SIZE/2):size//2+int(CROSS_SIZE/2),0] = 255
    return dst

if __name__ == "__main__":
    start = time.time()
    dst = generateImage(pitch = 45, roll = 90, heading = 45, size = 256, textureFile = 'NavBall_Texture.png')
    print("Time to generate image: ", time.time() - start)
    imageio.imwrite('result.png',dst)