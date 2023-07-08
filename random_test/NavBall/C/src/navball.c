/*
Fichier : navball.c
Auteur : Tanguy Dietrich
Description :
*/
#include "navball.h"
#include <stdio.h>
#include <stdlib.h>

// float px[SIZE_NAVBALL];
// float py[SIZE_NAVBALL];

//roll                     cs    0   ss      0    1    0      -ss   0   cs
static double ms[3][3] = {{0.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, {0.0, 0.0, 1.0}};
//pitch                     1     0   0       0    ct   st     0    -st  ct
static double mt[3][3] = {{1.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}};
//yaw                       cy    sy  0       -sy  cy   0      0    0   1
static double my[3][3] = {{0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}};

// float latitude[SIZE_NAVBALL][SIZE_NAVBALL];
// float longitude[SIZE_NAVBALL][SIZE_NAVBALL];
float hx[SIZE_NAVBALL][SIZE_NAVBALL];
float hy[SIZE_NAVBALL][SIZE_NAVBALL];
float hz[SIZE_NAVBALL][SIZE_NAVBALL];
// float xyz[SIZE_NAVBALL][SIZE_NAVBALL][3];
// float xyz2[SIZE_NAVBALL][SIZE_NAVBALL][3];
// float hx2[SIZE_NAVBALL][SIZE_NAVBALL];
// float hy2[SIZE_NAVBALL][SIZE_NAVBALL];
// float hz2[SIZE_NAVBALL][SIZE_NAVBALL];
// // float xyz3[SIZE_NAVBALL][SIZE_NAVBALL][3];
// float hx3[SIZE_NAVBALL][SIZE_NAVBALL];
// float hy3[SIZE_NAVBALL][SIZE_NAVBALL];
// float hz3[SIZE_NAVBALL][SIZE_NAVBALL];

void generateNavBall(imageRGB *texture, imageRGB *navballImage, float pitch, float roll, float yaw)
{
    double cs, ss, ct, st, cy, sy;
    float lat, lon;
    uint8_t r,g,b;
    int x, y;
    if (texture->width <= 0 || texture->height <= 0)
    {
        printf("Error texture image size is not valid \n");
        exit(1);
    }
    // create the navball
    // I consider the pitch, roll and yaw are in radian
    // Create two array px,py going from -1 to 1 with a step of 2/size
    // THIS STEP IS DONE IN THE MESHGRID FUNCTION
    // generatePixelArray(px, navballImage->width);
    // generatePixelArray(py, navballImage->height);
    // generatePixelXY(px, py);

    // create the meshgrid hx, hy
    meshgrid(hx, hy);

    // create the meshgrid hz and hit
    // compute_hz(hx, hy, hz, hit);
    compute_hz2(hx, hy, hz);


    cs = cos(roll);
    ss = sin(roll);
    ms[0][0] = cs;
    ms[0][2] = ss;
    ms[2][0] = -ss;
    ms[2][2] = cs;

    ct = cos(pitch);
    st = sin(pitch);
    mt[1][1] = ct;
    mt[1][2] = st;
    mt[2][1] = -st;
    mt[2][2] = ct;

    cy = cos(yaw);
    sy = sin(yaw);
    my[0][0] = cy;
    my[0][1] = sy;
    my[1][0] = -sy;
    my[1][1] = cy;

    // allocate xyz array of navballImage.size
    // dstack(xyz, hx, hy, hz, navballImage->width, navballImage->height, 3);
    // tensorDot(xyz, mt, navballImage->width, navballImage->height, 3, xyz2);
    // tensorDot2(hx, hy, hz, mt, hx2, hy2, hz2);
    tensorDot2InPlace(hx, hy, hz, mt);
    // tensorDot(xyz2, ms, navballImage->width, navballImage->height, 3, xyz3);
    // tensorDot2(hx2, hy2, hz2, ms, hx3, hy3, hz3); // i could reuse hx, hy, hz to save memory
    tensorDot2InPlace(hx, hy, hz, ms);
    // adding yaw
    tensorDot2InPlace(hx, hy, hz, my);
    

    // for (int i = 0; i < SIZE_NAVBALL; i++)
    // {
    //     for (int j = 0; j < SIZE_NAVBALL; j++)
    //     {
    //         if(hit[i][j] == 1)
    //         {
    //             latitude[i][j] = (0.5 + (asin(hy3[i][j])) / M_PI) * texture->height;
    //             longitude[i][j] = (1.0 + atan2(hz3[i][j], hx3[i][j]) / M_PI) * 0.5 * texture->width;
    //         }
    //         else
    //         {
    //             latitude[i][j] = 0.0;
    //             longitude[i][j] = 0.0;
    //         }
    //     }
    // }
    float r2;
    for (int i = 0; i < SIZE_NAVBALL; i++)
    {
        for (int j = 0; j < SIZE_NAVBALL; j++)
        {
            r2 = hx[i][j] * hx[i][j] + hy[i][j] * hy[i][j];
            if(r2 <= 1.0)
            {
                x = (int)((0.5 + (asin(hy[i][j])) / M_PI) * texture->height);
                y = (int)((1.0 + atan2(hz[i][j], hx[i][j]) / M_PI) * 0.5 * texture->width);
                r = (int) texture->data[x][y][0];
                g = (int) texture->data[x][y][1];
                b = (int) texture->data[x][y][2];
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

// float ***tensorDot(float ***xyz, double **m, int sizex, int sizey, int sizez)
void tensorDot(float xyz[SIZE_NAVBALL][SIZE_NAVBALL][3], double m[3][3], int sizex, int sizey, int sizez, float res[SIZE_NAVBALL][SIZE_NAVBALL][3])
{
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
}

// this function is used to avoid call to dstack, because we pass directly hx, hy, hz
void tensorDot2(float hx[SIZE_NAVBALL][SIZE_NAVBALL], float hy[SIZE_NAVBALL][SIZE_NAVBALL], float hz[SIZE_NAVBALL][SIZE_NAVBALL], double m[3][3], float r_hx[SIZE_NAVBALL][SIZE_NAVBALL], float r_hy[SIZE_NAVBALL][SIZE_NAVBALL], float r_hz[SIZE_NAVBALL][SIZE_NAVBALL])
{
    // in normal time i should transpose m, but id do it in the loop by inverting the indices
    // perform the dot product
    for (int i = 0; i < SIZE_NAVBALL; i++)
    {
        for (int j = 0; j < SIZE_NAVBALL; j++)
        {
            r_hx[i][j] = hx[i][j] * m[0][0] + hy[i][j] * m[0][1] + hz[i][j] * m[0][2];
            r_hy[i][j] = hx[i][j] * m[1][0] + hy[i][j] * m[1][1] + hz[i][j] * m[1][2];
            r_hz[i][j] = hx[i][j] * m[2][0] + hy[i][j] * m[2][1] + hz[i][j] * m[2][2];
        }
    }
}

void tensorDot2InPlace(float hx[SIZE_NAVBALL][SIZE_NAVBALL], float hy[SIZE_NAVBALL][SIZE_NAVBALL], float hz[SIZE_NAVBALL][SIZE_NAVBALL], double m[3][3])
{
    float tmp_hx = 0;
    float tmp_hy = 0;
    float tmp_hz = 0;
    for (int i = 0; i < SIZE_NAVBALL; i++)
    {
        for (int j = 0; j < SIZE_NAVBALL; j++)
        {
            tmp_hx = hx[i][j];
            tmp_hy = hy[i][j];
            tmp_hz = hz[i][j];
            hx[i][j] = tmp_hx * m[0][0] + tmp_hy * m[0][1] + tmp_hz * m[0][2];
            hy[i][j] = tmp_hx * m[1][0] + tmp_hy * m[1][1] + tmp_hz * m[1][2];
            hz[i][j] = tmp_hx * m[2][0] + tmp_hy * m[2][1] + tmp_hz * m[2][2];
        }
    }
}

void compute_hz(float hx[SIZE_NAVBALL][SIZE_NAVBALL], float hy[SIZE_NAVBALL][SIZE_NAVBALL], float hz[SIZE_NAVBALL][SIZE_NAVBALL], uint8_t hit[SIZE_NAVBALL][SIZE_NAVBALL])
{
    float r2;
    // compute hz and hit
    for (int i = 0; i < SIZE_NAVBALL; i++)
    {
        for (int j = 0; j < SIZE_NAVBALL; j++)
        {
            r2 = hx[i][j] * hx[i][j] + hy[i][j] * hy[i][j];
            hit[i][j] = r2 <= 1.0;
            if (hit[i][j])
            {
                // if hit the hz[i,j] = -np.sqrt(1.0-np.where(hit,r2,0.0)
                hz[i][j] = -sqrt(1.0 - r2); 
            }
            else
            {
                hz[i][j] = NAN;
            }
        }
    }
}

void compute_hz2(float hx[SIZE_NAVBALL][SIZE_NAVBALL], float hy[SIZE_NAVBALL][SIZE_NAVBALL], float hz[SIZE_NAVBALL][SIZE_NAVBALL])
{
    float r2;
    // compute hz and hit
    for (int i = 0; i < SIZE_NAVBALL; i++)
    {
        for (int j = 0; j < SIZE_NAVBALL; j++)
        {
            r2 = hx[i][j] * hx[i][j] + hy[i][j] * hy[i][j];
            if (r2 <= 1.0)
            {
                // if hit the hz[i,j] = -np.sqrt(1.0-np.where(hit,r2,0.0)
                hz[i][j] = -sqrt(1.0 - r2); 
            }
            else
            {
                hz[i][j] = NAN;
            }
        }
    }
}

void meshgrid(float hx[SIZE_NAVBALL][SIZE_NAVBALL], float hy[SIZE_NAVBALL][SIZE_NAVBALL])
{
    float step = 2.0 / (float)SIZE_NAVBALL;
    for (int i = 0; i < SIZE_NAVBALL; i++)
    {
        for (int j = 0; j < SIZE_NAVBALL; j++)
        {
            // hx[i][j] = px[j];
            hx[i][j] = -1.0 + (float)j * step + 1.0 / (float)SIZE_NAVBALL;
        }
    }
    for (int i = 0; i < SIZE_NAVBALL; i++)
    {
        for (int j = 0; j < SIZE_NAVBALL; j++)
        {
            // hy[i][j] = py[i];
            hy[i][j] = -1.0 + (float)i * step + 1.0 / (float)SIZE_NAVBALL;
        }
    }
}

void generatePixelArray(float px[SIZE_NAVBALL], uint32_t size)
{
    // equivalent to px=np.arange(-1.0,1.0,2.0/size)+1.0/size
    float step = 2.0 / (float)size;
    for (uint32_t i = 0; i < size; i++)
    {
        px[i] = -1.0 + (float)i * step + 1.0 / (float)size;
    }
}

void generatePixelXY(float px[SIZE_NAVBALL], float py[SIZE_NAVBALL])
{
    // equivalent to calling generatePixelArray twice
    float step = 2.0 / (float)SIZE_NAVBALL;
    for (uint32_t i = 0; i < SIZE_NAVBALL; i++)
    {
        px[i] = -1.0 + (float)i * step + 1.0 / (float)SIZE_NAVBALL;
        py[i] = -1.0 + (float)i * step + 1.0 / (float)SIZE_NAVBALL;
    }
}


void dstack(float xyz[SIZE_NAVBALL][SIZE_NAVBALL][3], float hx[SIZE_NAVBALL][SIZE_NAVBALL], float hy[SIZE_NAVBALL][SIZE_NAVBALL], float hz[SIZE_NAVBALL][SIZE_NAVBALL], int sizex, int sizey, int sizez)
{
    for (int i = 0; i < sizex; i++)
    {
        for (int j = 0; j < sizey; j++)
        {
            xyz[i][j][0] = hx[i][j];
            xyz[i][j][1] = hy[i][j];
            xyz[i][j][2] = hz[i][j];
        }
    }
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