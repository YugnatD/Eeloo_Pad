#ifndef NAVBALL_H
#define NAVBALL_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <math.h>

#include "image.h"

float *generatePixelArray(uint32_t size);
void meshgrid(float *px, float *py, float ***hx, float ***hy, int sizex, int sizey);
float ***dstack(float **hx, float **hy, float **hz, int sizex, int sizey, int sizez);
void generateNavBall(imageRGB *texture, imageRGB *navballImage, float pitch, float roll, float yaw);

#endif
