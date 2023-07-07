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
    // print1dArray(px, navballImage->width);
    // print1dArray(py, navballImage->height);
    // create the meshgrid hx, hy
    float **hx = NULL;
    float **hy = NULL;
    meshgrid(px, py, &hx, &hy, navballImage->width, navballImage->height);
    // print2dArray(hx, navballImage->width, navballImage->height);
    // print2dArray(hy, navballImage->width, navballImage->height);

    // create the meshgrid hz and hit
    float **hz = NULL;
    uint8_t **hit = NULL;
    compute_hz(hx, hy, &hz, &hit, navballImage->width, navballImage->height);
    // print2dArray(hz, navballImage->width, navballImage->height);
    // print2dArrayUint8(hit, navballImage->width, navballImage->height);

    double cs = cos(roll);
    double ss = sin(roll);
    // double ms[3][3] = {{cs, 0.0, ss}, {0.0, 1.0, 0.0}, {-ss, 0.0, cs}};
    double **ms = (double **)malloc(3 * sizeof(double *));
    for (int i = 0; i < 3; i++)
    {
        ms[i] = (double *)malloc(3 * sizeof(double));
    }
    ms[0][0] = cs;
    ms[0][1] = 0.0;
    ms[0][2] = ss;
    ms[1][0] = 0.0;
    ms[1][1] = 1.0;
    ms[1][2] = 0.0;
    ms[2][0] = -ss;
    ms[2][1] = 0.0;
    ms[2][2] = cs;
    // print2dArrayDouble(ms, 3, 3);

    double ct = cos(pitch);
    double st = sin(pitch);
    // double mt[3][3] = {{1.0, 0.0, 0.0}, {0.0, ct, st}, {0.0, -st, ct}};
    double **mt = (double **)malloc(3 * sizeof(double *));
    for (int i = 0; i < 3; i++)
    {
        mt[i] = (double *)malloc(3 * sizeof(double));
    }
    mt[0][0] = 1.0;
    mt[0][1] = 0.0;
    mt[0][2] = 0.0;
    mt[1][0] = 0.0;
    mt[1][1] = ct;
    mt[1][2] = st;
    mt[2][0] = 0.0;
    mt[2][1] = -st;
    mt[2][2] = ct;
    // print2dArrayDouble(mt, 3, 3);

    // allocate xyz array of navballImage.size
    float ***xyz = dstack(hx, hy, hz, navballImage->width, navballImage->height, 3);
    // print3dArray(xyz, navballImage->width, navballImage->height, 3);
    float ***xyz2 = tensorDot(xyz, mt, navballImage->width, navballImage->height, 3);
    // print3dArray(xyz2, navballImage->width, navballImage->height, 3);
    float ***xyz3 = tensorDot(xyz2, ms, navballImage->width, navballImage->height, 3);
    // print3dArray(xyz3, navballImage->width, navballImage->height, 3);
    float lat=0.0;
    float lon=0.0;
    uint8_t r,g,b;
    printf("texture->height: %d, texture->width: %d \r\n", texture->height, texture->width);
    // int x = 1024-315;
    // int y = 512-125;
    int x = 1024-125;
    int y = 512-315;
    // printf("texture[%d,%d] = %d %d %d \r\n", x, y, texture->data[x][y][0], texture->data[x][y][1], texture->data[x][y][2]);

    float **latitude = (float **)malloc(navballImage->width * sizeof(float *));
    for (int i = 0; i < navballImage->width; i++)
    {
        latitude[i] = (float *)malloc(navballImage->height * sizeof(float));
    }

    float **longitude = (float **)malloc(navballImage->width * sizeof(float *));
    for (int i = 0; i < navballImage->width; i++)
    {
        longitude[i] = (float *)malloc(navballImage->height * sizeof(float));
    }

    for (int i = 0; i < navballImage->width; i++)
    {
        for (int j = 0; j < navballImage->height; j++)
        {
            if(hit[i][j] == 1)
            {
                latitude[i][j] = (0.5 + (asin(xyz3[i][j][1])) / M_PI) * texture->height;
                longitude[i][j] = (1.0 + atan2(xyz3[i][j][2], xyz3[i][j][0]) / M_PI) * 0.5 * texture->width;
            }
            else
            {
                latitude[i][j] = 0.0;
                longitude[i][j] = 0.0;
            }
        }
    }

    // print lat and lon at specific pixel for debugging
    printf("latitude[%d,%d] = %f, longitude[%d,%d] = %f \r\n", 125, 125, latitude[125][125], 125, 125, longitude[125][125]);
    
    // print pixel x,y of texture
    x = 378;
    y = 18;
    printf("texture[%d,%d] = %d %d %d \r\n", x, y, texture->data[x][y][0], texture->data[x][y][1], texture->data[x][y][2]);

    // print 
    for (int i = 0; i < navballImage->width; i++)
    {
        for (int j = 0; j < navballImage->height; j++)
        {
            if(hit[i][j] == 1)
            {
                x = (int)latitude[i][j] % texture->height;
                y = (int)longitude[i][j] % texture->width;
                // x = (texture->width - x);
                // y = (texture->height - y);
                // printf("i = %d, j = %d, x = %d, y = %d \r\n", i, j, x, y);
                // if (x < 0 || x >= texture->width || y < 0 || y >= texture->height)
                // {
                //     // set the color to black
                //     navballImage->data[i][j][0] = 0;
                //     navballImage->data[i][j][1] = 0;
                //     navballImage->data[i][j][2] = 0;
                //     continue;
                // }
                // r = (int) texture->data[y][x][0];
                // g = (int) texture->data[y][x][1];
                // b = (int) texture->data[y][x][2];
                r = (int) texture->data[x][y][0];
                g = (int) texture->data[x][y][1];
                b = (int) texture->data[x][y][2];
                // print all data x,y,r,g,b,latitude,longitude
                // printf("%d, %d, %d, %d, %d, %f, %f \r\n", x, y, r, g, b, latitude[i][j], longitude[i][j]);
                navballImage->data[i][j][0] = r;
                navballImage->data[i][j][1] = g;
                navballImage->data[i][j][2] = b;
            }
            else
            {
                // set the color to black
                navballImage->data[i][j][0] = 0;
                navballImage->data[i][j][1] = 0;
                navballImage->data[i][j][2] = 0;
            }
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
    free(hz);
    free(hit);
    for (int i = 0; i < 3; i++)
    {
        free(ms[i]);
    }
    free(ms);
    for (int i = 0; i < 3; i++)
    {
        free(mt[i]);
    }
    free(mt);
}

void bilinear(float x, float y, imageRGB *texture, uint8_t *r, uint8_t *g, uint8_t *b)
{
    
    int x1 = floor(x);
    int x2 = ceil(x);
    int y1 = floor(y);
    int y2 = ceil(y);
    // check if the point is on the image
    if(x1 >= texture->width)
    {
        x1 = texture->width - 1;
        printf("x1 is out of range %d\r\n", x1);
    }
    if(x2 >= texture->width)
    {
        x2 = texture->width - 1;
        printf("x2 is out of range %d\r\n", x2);
    }
    if(y1 >= texture->height)
    {
        y1 = texture->height - 1;
        printf("y1 is out of range %d\r\n", y1);
    }
    if(y2 >= texture->height)
    {
        y2 = texture->height - 1;
        printf("y2 is out of range %d\r\n", y2);
    }
    float x1y1 = (x2 - x) * (y2 - y);
    float x2y1 = (x - x1) * (y2 - y);
    float x1y2 = (x2 - x) * (y - y1);
    float x2y2 = (x - x1) * (y - y1);
    *r = x1y1 * texture->data[x1][y1][0] + x2y1 * texture->data[x2][y1][0] + x1y2 * texture->data[x1][y2][0] + x2y2 * texture->data[x2][y2][0];
    *g = x1y1 * texture->data[x1][y1][1] + x2y1 * texture->data[x2][y1][1] + x1y2 * texture->data[x1][y2][1] + x2y2 * texture->data[x2][y2][1];
    *b = x1y1 * texture->data[x1][y1][2] + x2y1 * texture->data[x2][y1][2] + x1y2 * texture->data[x1][y2][2] + x2y2 * texture->data[x2][y2][2];
}

// equivalent to scipy.ndimage.interpolation.map_coordinates
void interpolationMapCoordinate(float lat, float lon, imageRGB *texture, uint8_t *r, uint8_t *g, uint8_t *b)
{

}

float ***tensorDot(float ***xyz, double **m, int sizex, int sizey, int sizez)
{
    // equivalent to xyz=np.tensordot(xyz,mt,axes=([2],[1]))
    // i consider the axis to be fixed to (2,1)
    // allocate the result
    float ***res = (float ***)malloc(sizex * sizeof(float **));
    for (int i = 0; i < sizex; i++)
    {
        res[i] = (float **)malloc(sizey * sizeof(float *));
        for (int j = 0; j < sizey; j++)
        {
            res[i][j] = (float *)malloc(sizez * sizeof(float));
        }
    }
    // in normal time i should transpose m, but id do it in the loop by inverting the indices
    // perform the dot product
    for (int i = 0; i < sizex; i++)
    {
        for (int j = 0; j < sizey; j++)
        {
            for (int k = 0; k < sizez; k++)
            {
                res[i][j][k] = xyz[i][j][0] * m[k][0] + xyz[i][j][1] * m[k][1] + xyz[i][j][2] * m[k][2];
            }
        }
    }
    return res;
}

void compute_hz(float **hx, float **hy, float ***hz, uint8_t ***hit, int sizex, int sizey)
{
    //allocate hz and hit
    float r2;
    *hz = malloc(sizex * sizeof(float *));
    *hit = malloc(sizex * sizeof(uint8_t *)); // hit is a boolean
    for (int i = 0; i < sizex; i++)
    {
        (*hz)[i] = malloc(sizey * sizeof(float));
        (*hit)[i] = malloc(sizey * sizeof(uint8_t));
    }
    // compute hz and hit
    for (int i = 0; i < sizex; i++)
    {
        for (int j = 0; j < sizey; j++)
        {
            r2 = hx[i][j] * hx[i][j] + hy[i][j] * hy[i][j];
            (*hit)[i][j] = r2 <= 1.0;
            if ((*hit)[i][j])
            {
                // if hit the hz[i,j] = -np.sqrt(1.0-np.where(hit,r2,0.0)
                (*hz)[i][j] = -sqrt(1.0 - r2); // NOT SURE
            }
            else
            {
                (*hz)[i][j] = NAN;
            }
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
    for (uint32_t i = 0; i < size; i++)
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

void print3dArray(float ***array, int sizex, int sizey, int sizez)
{
    for (int i = 0; i < sizex; i++)
    {
        printf("[");
        for (int j = 0; j < sizey; j++)
        {
            printf("[");
            for (int k = 0; k < sizez; k++)
            {
                printf("%f ", array[i][j][k]);
            }
            printf("]");
        }
        printf("]\r\n");
    }
    printf("\r\n");
}

void print2dArray(float **array, int sizex, int sizey)
{
    for (int i = 0; i < sizex; i++)
    {
        printf("[");
        for (int j = 0; j < sizey; j++)
        {
            printf("%f ", array[i][j]);
        }
        printf("]\r\n");
    }
    printf("\r\n");
}

void print1dArray(float *array, int size)
{
    for (int i = 0; i < size; i++)
    {
        printf("%f ", array[i]);
    }
    printf("\r\n");
}

void print2dArrayUint8(uint8_t **array, int sizex, int sizey)
{
    for (int i = 0; i < sizex; i++)
    {
        printf("[");
        for (int j = 0; j < sizey; j++)
        {
            printf("%d ", array[i][j]);
        }
        printf("]\r\n");
    }
    printf("\r\n");
}

void print2dArrayDouble(double **array, int sizex, int sizey)
{
    for (int i = 0; i < sizex; i++)
    {
        printf("[");
        for (int j = 0; j < sizey; j++)
        {
            printf("%f ", array[i][j]);
        }
        printf("]\r\n");
    }
    printf("\r\n");
}