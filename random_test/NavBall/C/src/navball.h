#ifndef NAVBALL_H
#define NAVBALL_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <math.h>
#include <time.h>
// #include <SDL2/SDL.h>
#include <pthread.h>

#include "textureMap.h"

#define NUM_THEARD 24

#define SIZE_NAVBALL 256
#define STEP (2.0 / (float)SIZE_NAVBALL)

#define SIZE_PRECOMPUTED_VALUES_SIN_COS 256
#define CONVERT_RAD_TO_PRE (SIZE_PRECOMPUTED_VALUES_SIN_COS / M_PI)

#define COMMA_FIX 8 // full size is 32 bit
#define MULTIPLIER (1 << COMMA_FIX)

#define ENABLE_PRE_COMPUTED_SIN_COS_VALUES


typedef struct _navballImage {
    // int width; // USE DEFINE
    // int height;
  uint8_t data[SIZE_NAVBALL][SIZE_NAVBALL][3];
} navballImage_t;

typedef struct _param_navball_thread {
  int id;
  textureMap_t *texture;
  navballImage_t *navballImage;
  double *cs;
  double *ss;
  double *ct;
  double *st;
  double *cy;
  double *sy;
} param_navball_thread_t;

// #include "image.h"
void generateNavBall(textureMap_t *texture, navballImage_t *navballImage, float pitch, float roll, float yaw);
// void generateNavBallFast(textureMap_t *texture, navballImage_t *navballImage, int pitch, int roll, int yaw);
void initPreComputedValue();
void *generateNavBallThread(void *arg);
void generateNavBallMt(textureMap_t *texture, navballImage_t *navballImage, float pitch, float roll, float yaw);
void savePPM(navballImage_t *image, char *filename);
double FastArcTan(double x);
double FastArcTan2(double y, double x);
double FastASin(double x);
double FastCos (double x);
double FastSin (double x);

// FUNCTION NOT USED ANYMORE
void bilinear(float x, float y, textureMap_t *texture, uint8_t *r, uint8_t *g, uint8_t *b);
void generatePixelArray(float px[SIZE_NAVBALL], uint32_t size);
void generatePixelXY(float px[SIZE_NAVBALL], float py[SIZE_NAVBALL]);
void tensorDot(float xyz[SIZE_NAVBALL][SIZE_NAVBALL][3], double m[3][3], int sizex, int sizey, int sizez, float res[SIZE_NAVBALL][SIZE_NAVBALL][3]);
void tensorDot2(float hx[SIZE_NAVBALL][SIZE_NAVBALL], float hy[SIZE_NAVBALL][SIZE_NAVBALL], float hz[SIZE_NAVBALL][SIZE_NAVBALL], double m[3][3], float r_hx[SIZE_NAVBALL][SIZE_NAVBALL], float r_hy[SIZE_NAVBALL][SIZE_NAVBALL], float r_hz[SIZE_NAVBALL][SIZE_NAVBALL]);
void tensorDot2InPlace(float hx[SIZE_NAVBALL][SIZE_NAVBALL], float hy[SIZE_NAVBALL][SIZE_NAVBALL], float hz[SIZE_NAVBALL][SIZE_NAVBALL], double m[3][3]);
void meshgrid(float hx[SIZE_NAVBALL][SIZE_NAVBALL], float hy[SIZE_NAVBALL][SIZE_NAVBALL]);
void dstack(float xyz[SIZE_NAVBALL][SIZE_NAVBALL][3], float hx[SIZE_NAVBALL][SIZE_NAVBALL], float hy[SIZE_NAVBALL][SIZE_NAVBALL], float hz[SIZE_NAVBALL][SIZE_NAVBALL], int sizex, int sizey, int sizez);
void compute_hz(float hx[SIZE_NAVBALL][SIZE_NAVBALL], float hy[SIZE_NAVBALL][SIZE_NAVBALL], float hz[SIZE_NAVBALL][SIZE_NAVBALL], uint8_t hit[SIZE_NAVBALL][SIZE_NAVBALL]);
void compute_hz2(float hx[SIZE_NAVBALL][SIZE_NAVBALL], float hy[SIZE_NAVBALL][SIZE_NAVBALL], float hz[SIZE_NAVBALL][SIZE_NAVBALL]);
// void convolve2dInPlace(float ***img, double **k, int sizex, int sizey, int sizez);
void print3dArray(float ***array, int sizex, int sizey, int sizez);
void print2dArray(float **array, int sizex, int sizey);
void print1dArray(float *array, int size);
void print2dArrayUint8(uint8_t **array, int sizex, int sizey);
void print2dArrayDouble(double **array, int sizex, int sizey);
#endif
