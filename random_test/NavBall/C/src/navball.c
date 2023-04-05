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
    // TODO Optimize to get one loop, the image is a square...
    float *px = generatePixelArray(navballImage->width);
    float *py = generatePixelArray(navballImage->height);
    // printf("[DEBUG] px[55] = %f \r\n", px[55]);
    // printf("[DEBUG] py[42] = %f \r\n", py[42]);
    // create the meshgrid hx, hy
    float **hx = NULL;
    float **hy = NULL;
    meshgrid(px, py, &hx, &hy, navballImage->width, navballImage->height);

    // printf("[DEBUG] hx[55][42] = %f \r\n", hx[77][39]);
    // printf("[DEBUG] hy[55][42] = %f \r\n", hy[39][77]);
    // create the meshgrid hz
    float **hz = malloc(navballImage->width * sizeof(float *));
    for (int i = 0; i < navballImage->width; i++)
    {
        hz[i] = malloc(navballImage->height * sizeof(float));
        for (int j = 0; j < navballImage->height; j++)
        {
            hz[i][j] = sqrt(1.0 - hx[i][j] * hx[i][j] - hy[i][j] * hy[i][j]);
        }
    }
    printf("[DEBUG] hz[55][42] = %f \r\n", hz[55][42]);

    double cs = cos(roll);
    double ss = sin(roll);
    double ms[3][3] = {{cs, 0.0, ss}, {0.0, 1.0, 0.0}, {-ss, 0.0, cs}};

    double ct = cos(pitch);
    double st = sin(pitch);
    double mt[3][3] = {{1.0, 0.0, 0.0}, {0.0, ct, st}, {0.0, -st, ct}};

    // allocate xyz array of navballImage.size
    float ***xyz = dstack(hx, hy, hz, navballImage->width, navballImage->height, 3);

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
    // free xyz
    for (int i = 0; i < navballImage->width; i++)
    {
        for (int j = 0; j < navballImage->height; j++)
        {
            free(xyz[i][j]);
        }
        free(xyz[i]);
    }
    free(xyz);
}

void compute_hz(float **hx, float **hy, float **hz, int sizex, int sizey)
{
    for (int i = 0; i < sizex; i++)
    {
        for (int j = 0; j < sizey; j++)
        {
            hz[i][j] = sqrt(1.0 - hx[i][j] * hx[i][j] - hy[i][j] * hy[i][j]);
        }
    }
}

void meshgrid(float *px, float *py, float ***hx, float ***hy, int sizex, int sizey)
{
    // equivalent to hx,hy=scipy.meshgrid(px,py)
    *hx = malloc(sizex * sizeof(float *));
    *hy = malloc(sizey * sizeof(float *));
    for (int i = 0; i < sizex; i++)
    {
        (*hx)[i] = malloc(sizey * sizeof(float));
        for (int j = 0; j < sizey; j++)
        {
            (*hx)[i][j] = px[j];
        }
    }
    for (int i = 0; i < sizey; i++)
    {
        (*hy)[i] = malloc(sizex * sizeof(float));
        for (int j = 0; j < sizex; j++)
        {
            (*hy)[i][j] = py[i];
        }
    }
}

float *generatePixelArray(uint32_t size)
{
    // equivalent to px=np.arange(-1.0,1.0,2.0/size)+1.0/size
    float *px = malloc(size * sizeof(float));
    float step = 2.0 / (float)size;
    for (int i = 0; i < size; i++)
    {
        px[i] = -1.0 + (float)i * step + 1.0 / (float)size;
    }
    return px;
}


float ***dstack(float **hx, float **hy, float **hz, int sizex, int sizey, int sizez)
{
    float ***xyz;
    // sizex = navballImage->width
    // sizey = navballImage->height
    // sizez = 3
    xyz = malloc(sizex * sizeof(float **));
    for (int i = 0; i < sizex; i++)
    {
        xyz[i] = malloc(sizey * sizeof(float *));
        for (int j = 0; j < sizey; j++)
        {
            xyz[i][j] = malloc(3 * sizeof(float));
        }
    }
    for (int i = 0; i < sizex; i++)
    {
        for (int j = 0; j < sizey; j++)
        {
            xyz[i][j][0] = hx[i][j];
            xyz[i][j][1] = hy[i][j];
            xyz[i][j][2] = hz[i][j];
        }
    }
    return xyz;
}
