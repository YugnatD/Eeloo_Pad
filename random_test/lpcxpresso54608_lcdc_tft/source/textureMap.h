#ifndef TEXTURE_MAP_H
#define TEXTURE_MAP_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <cr_section_macros.h>

#include <math.h>


// we work on uC, so we use static array with fixed size
#define TEXTURE_MAP_WIDTH 1024
#define TEXTURE_MAP_HEIGHT 512

typedef struct _textureMap {
    uint16_t width; // USE DEFINE, it's here because of gimp export
    uint16_t height;
    uint8_t bytes_per_pixel; /* 2:RGB16, 3:RGB, 4:RGBA */ 
//   uint8_t data[TEXTURE_MAP_HEIGHT][TEXTURE_MAP_WIDTH][3];
    uint8_t data[TEXTURE_MAP_HEIGHT * TEXTURE_MAP_WIDTH * 3 + 1];
} textureMap_t;


void openTextureMap(textureMap_t *textureMap, char *filename);

#endif
