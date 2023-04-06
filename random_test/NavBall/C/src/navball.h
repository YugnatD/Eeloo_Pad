#ifndef NAVBALL_H
#define NAVBALL_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <math.h>

#include "image.h"

void bilinear(float x, float y, imageRGB *texture, uint8_t *r, uint8_t *g, uint8_t *b);
float *generatePixelArray(uint32_t size);
float ***tensorDot(float ***xyz, double **m, int sizex, int sizey, int sizez);
void meshgrid(float *px, float *py, float ***hx, float ***hy, int sizex, int sizey);
float ***dstack(float **hx, float **hy, float **hz, int sizex, int sizey, int sizez);
void generateNavBall(imageRGB *texture, imageRGB *navballImage, float pitch, float roll, float yaw);
void compute_hz(float **hx, float **hy, float ***hz, uint8_t ***hit, int sizex, int sizey);
// void convolve2dInPlace(float ***img, double **k, int sizex, int sizey, int sizez);
void print3dArray(float ***array, int sizex, int sizey, int sizez);
void print2dArray(float **array, int sizex, int sizey);
void print1dArray(float *array, int size);
void print2dArrayUint8(uint8_t **array, int sizex, int sizey);
void print2dArrayDouble(double **array, int sizex, int sizey);
#endif
