#ifndef TEXTURE_MAP_H
#define TEXTURE_MAP_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <math.h>


// we work on uC, so we use static array with fixed size
#define TEXTURE_MAP_WIDTH 1024
#define TEXTURE_MAP_HEIGHT 512

#define DEMI_TEXTURE_MAP_WIDTH (TEXTURE_MAP_WIDTH / 2)
#define DEMI_TEXTURE_MAP_HEIGHT (TEXTURE_MAP_HEIGHT / 2)

#define X2_TEXTURE_MAP_WIDTH (2 * TEXTURE_MAP_WIDTH)
#define X2_TEXTURE_MAP_HEIGHT (2 * TEXTURE_MAP_WIDTH)

#define TEXTURE_MAP_HEIGHT_PI (TEXTURE_MAP_HEIGHT / M_PI)
#define DEMI_TEXTURE_MAP_WIDTH_PI (DEMI_TEXTURE_MAP_WIDTH / M_PI)
// #define TEXTURE_MAP_WIDTH 612
// #define TEXTURE_MAP_HEIGHT 306

// #define TEXTURE_MAP_WIDTH 400
// #define TEXTURE_MAP_HEIGHT 200

typedef struct _textureMap {
    uint16_t width; // USE DEFINE, it's here because of gimp export
    uint16_t height;
    uint8_t bytes_per_pixel; /* 2:RGB16, 3:RGB, 4:RGBA */ 
//   uint8_t data[TEXTURE_MAP_HEIGHT][TEXTURE_MAP_WIDTH][3];
    uint8_t data[TEXTURE_MAP_HEIGHT * TEXTURE_MAP_WIDTH * 3 + 1];
} textureMap_t;


void openTextureMap(textureMap_t *textureMap, char *filename);

#endif
