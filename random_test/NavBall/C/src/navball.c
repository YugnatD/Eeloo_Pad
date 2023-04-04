/*
Fichier : navball.c
Auteur : Tanguy Dietrich
Description :
*/
#include "navball.h"
#include <stdio.h>
#include <stdlib.h>

void generateNavBall(imageRGB *texture, imageRGB *navballImage, float pitch, float roll, float yaw)
{
    if (texture->width <= 0 || texture->height <= 0)
    {
        printf("Error texture image size is not valid \n");
        exit(1);
    }
    if (navballImage->width <= 0 || navballImage->height <= 0)
    {
        printf("Error navball image size is not valid \n");
        exit(1);
    }
    // create the navball
    // I consider the pitch, roll and yaw are in radian
    // Create two array px,py going from -1 to 1 with a step of 2/size
    float *px = malloc(navballImage->width * sizeof(float));
    float *py = malloc(navballImage->height * sizeof(float));
    float step = 2.0 / (float)navballImage->width;
    // TODO Optimize to get one loop, the image is a square...
    // no need to loop twice
    for (int i = 0; i < navballImage->width; i++)
    {
        px[i] = -1.0 + (float)i * step;
    }
    step = 2.0 / (float)navballImage->height;
    for (int i = 0; i < navballImage->height; i++)
    {
        py[i] = -1.0 + (float)i * step;
    }
    // create the meshgrid hx,hy
    float **hx = malloc(navballImage->width * sizeof(float *));
    float **hy = malloc(navballImage->height * sizeof(float *));
    for (int i = 0; i < navballImage->width; i++)
    {
        hx[i] = malloc(navballImage->height * sizeof(float));
        for (int j = 0; j < navballImage->height; j++)
        {
            hx[i][j] = px[i];
        }
    }
    for (int i = 0; i < navballImage->height; i++)
    {
        hy[i] = malloc(navballImage->width * sizeof(float));
        for (int j = 0; j < navballImage->width; j++)
        {
            hy[i][j] = py[i];
        }
    }
    // create the meshgrid hz
    for (int i = 0; i < navballImage->width; i++)
    {
        for (int j = 0; j < navballImage->height; j++)
        {
            float x = hx[i][j];
            float y = hy[i][j];
            float z = sqrt(1.0 - x * x - y * y);
            if (z < 0.0)
            {
                z = 0.0;
            }
            // compute the texture coordinates
            float u = 0.5 + atan2(x, z) / (2.0 * M_PI);
            float v = 0.5 - asin(y) / M_PI;
            // get the texture color
            int32_t xTexture = (int32_t)(u * (float)texture->width);
            int32_t yTexture = (int32_t)(v * (float)texture->height);
            // set the navball color
            navballImage->data[i][j][0] = texture->data[xTexture][yTexture][0];
            navballImage->data[i][j][1] = texture->data[xTexture][yTexture][1];
            navballImage->data[i][j][2] = texture->data[xTexture][yTexture][2];
        }
    }

    // free the memory
    free(px);
    free(py);
    for (int i = 0; i < navballImage->width; i++)
    {
        free(hx[i]);
    }
    free(hx);
    for (int i = 0; i < navballImage->height; i++)
    {
        free(hy[i]);
    }
    free(hy);
}

