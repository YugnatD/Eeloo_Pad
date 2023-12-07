#ifndef NAVBALL_H
#define NAVBALL_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <math.h>

#include "textureMap.h"

#define SIZE_NAVBALL 256
#define STEP (2.0 / (float)SIZE_NAVBALL)

#define SIZE_PRECOMPUTED_VALUES_SIN_COS 256
#define CONVERT_RAD_TO_PRE (SIZE_PRECOMPUTED_VALUES_SIN_COS / M_PI)

#define COMMA_FIX 8 // full size is 32 bit
#define MULTIPLIER (1 << COMMA_FIX)

#define ENABLE_PRECOMPUTED_VALUES
#define ENABLE_PRE_COMPUTED_SIN_COS_VALUES



// VALUE FOR FIXED POINT
#define FP_ASIN_1 (4.5334220547132049e-2 * MULTIPLIER) 
#define FP_ASIN_2 (1.1226216762576600e-2 * MULTIPLIER)
#define FP_ASIN_3 (2.6334281471361822e-2 * MULTIPLIER)
#define FP_ASIN_4 (2.0596336163223834e-2 * MULTIPLIER)
#define FP_ASIN_5 (3.0582043602875735e-2 * MULTIPLIER)
#define FP_ASIN_6 (4.4630538556294605e-2 * MULTIPLIER)
#define FP_ASIN_7 (7.5000364034134126e-2 * MULTIPLIER)
#define FP_ASIN_8 (1.6666666300567365e-1 * MULTIPLIER)

#define FP_COS_1 (-2.7236370439787708e-7 * MULTIPLIER)
#define FP_COS_2 (2.4799852696610628e-5 * MULTIPLIER)
#define FP_COS_3 (-1.3888885054799695e-3 * MULTIPLIER)
#define FP_COS_4 (4.1666666636943683e-2 * MULTIPLIER)
#define FP_COS_5 (-4.9999999999963024e-1 * MULTIPLIER)
#define FP_COS_6 (1.0000000000000000e+0 * MULTIPLIER)

#define FP_SIN_1 (2.7181216275479732e-6 * MULTIPLIER)
#define FP_SIN_2 (1.9839312269456257e-4 * MULTIPLIER)
#define FP_SIN_3 (8.3333293048425631e-3 * MULTIPLIER)
#define FP_SIN_4 (1.6666666640797048e-1 * MULTIPLIER)

#define FP_ATAN_1 (0.2447 * MULTIPLIER)
#define FP_ATAN_2 (0.0663 * MULTIPLIER)

#define FP_M_PI (M_PI * MULTIPLIER)
#define FP_M_PI_2 (M_PI_2 * MULTIPLIER)
#define FP_M_PI_4 (M_PI_4 * MULTIPLIER)


typedef struct _navballImage {
    // int width; // USE DEFINE
    // int height;
  uint8_t data[SIZE_NAVBALL][SIZE_NAVBALL][3];
} navballImage_t;


void generateNavBallFixed(textureMap_t *texture, navballImage_t *navballImage, int32_t pitch, int32_t roll, int32_t yaw);
int32_t fastASinFixedPoint(int32_t x);
int32_t FastCosFixedPoint(int32_t x);
int32_t FastSinFixedPoint(int32_t x);
int32_t FastArcTanFixedPoint(int32_t x);
int32_t FastArcTan2FixedPoint(int32_t y, int32_t x);

// #include "image.h"
void generateNavBall(textureMap_t *texture, navballImage_t *navballImage, float pitch, float roll, float yaw);
void initPreComputedValue();
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
