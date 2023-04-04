#ifndef IMAGE_H
#define IMAGE_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>

// #define WIDTH 256
// #define HEIGHT 256
// #define DEPTH 3 // RGB

typedef struct _imageRGB {
    int width;
    int height;
    int ***data;
//   int data[256][256][3]; // will be on a uC later, use static array
} imageRGB; // RGB Image

void createImageRGB(imageRGB *image, uint32_t width, uint32_t height);
void freeImageRGB(imageRGB *image);
void openPPM(imageRGB *image, char *filename);
void savePPM(imageRGB *image, char *filename);

#endif
