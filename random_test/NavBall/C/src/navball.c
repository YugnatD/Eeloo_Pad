/*
Fichier : navball.c
Auteur : Tanguy Dietrich
Description :
*/
#include "navball.h"
#include <stdio.h>
#include <stdlib.h>

#define ATAN2_LUT_SIZE 128
#define ASIN_LUT_SIZE 256

static float g_px_hx[SIZE_NAVBALL][SIZE_NAVBALL];
static float g_px_hy[SIZE_NAVBALL][SIZE_NAVBALL];
static float g_px_hz[SIZE_NAVBALL][SIZE_NAVBALL];
// static float g_r2[SIZE_NAVBALL][SIZE_NAVBALL];
static uint8_t g_r2[SIZE_NAVBALL][SIZE_NAVBALL];

static float g_sin[SIZE_PRECOMPUTED_VALUES_SIN_COS];
static float g_cos[SIZE_PRECOMPUTED_VALUES_SIN_COS];

float g_atan2[ATAN2_LUT_SIZE][ATAN2_LUT_SIZE];
float g_asin[ASIN_LUT_SIZE];


#define RANGE_ATAN2 1.0 // Define the range of x and y values (from -RANGE to +RANGE)
#define ATAN2_HALF_LUT_SIZE (ATAN2_LUT_SIZE / 2)

#define RANGE_ASIN 1.0

// Precompute the table
void init_g_atan2() {
    double x, y;
    double step = (double)RANGE_ATAN2 / (ATAN2_LUT_SIZE / 2);
    for (int i = 0; i < ATAN2_LUT_SIZE; ++i) {
        for (int j = 0; j < ATAN2_LUT_SIZE; ++j) {
            x = i * step - RANGE_ATAN2;
            y = j * step - RANGE_ATAN2;
            g_atan2[i][j] = atan2(y, x);
        }
    }
}

// Get index in the array for the given coordinate
int get_index_atan2(double coord) {
    int index = (int)((coord + RANGE_ATAN2) / (2 * RANGE_ATAN2) * ATAN2_LUT_SIZE);
    if (index < 0) return 0;
    if (index >= ATAN2_LUT_SIZE) return ATAN2_LUT_SIZE - 1;
    return index;
}

// Use the table
float fastAtan2(float y, float x) {
    int x_index = get_index_atan2(x);
    int y_index = get_index_atan2(y);
    return g_atan2[x_index][y_index];
}


// Precompute the table
// Precompute asin values for the array
void init_g_asin() {
    double step = 2 * RANGE_ASIN / (ASIN_LUT_SIZE - 1);
    for (int i = 0; i < ASIN_LUT_SIZE; ++i) {
        double x = i * step - RANGE_ASIN;
        g_asin[i] = (float)asin(x);
    }
}

// Get index in the array for the given input value
int get_index_asin(double x) {
    int index = (int)((x + RANGE_ASIN) / (2 * RANGE_ASIN) * (ASIN_LUT_SIZE - 1));
    if (index < 0) return 0;
    if (index >= ASIN_LUT_SIZE) return ASIN_LUT_SIZE - 1;
    return index;
}

// Use the table
float fastAsin(float x) {
    int index = get_index_asin(x);
    return g_asin[index];
}


//TODO : take into account when value is above 2pi
float fastSin(float x) {
    //we need to take into account that the array contain only value between 0 and pi,
    // for value between pi and 2pi we just need to invert the sign
    // and take into account that x can be negative
    if (x < 0)
    {
        return -fastSin(-x);
    }
    // check if we are between pi and 2pi
    if (x > M_PI)
    {
        return -fastSin(x - M_PI);
    }
    int index = (int)(x * CONVERT_RAD_TO_PRE);
    return g_sin[index];
}

float fastCos(float x) {
    //we need to take into account that the array contain only value between 0 and pi,
    // for value between pi and 2pi we just need to invert the sign
    // and take into account that x can be negative
    if (x < 0)
    {
        return fastCos(-x);
    }
    // check if we are between pi and 2pi
    if (x > M_PI)
    {
        return -fastCos(x - M_PI);
    }
    int index = (int)(x * CONVERT_RAD_TO_PRE);
    return g_cos[index];
}

void initPreComputedValue()
{
    float r2;
    for (int i = 0; i < SIZE_NAVBALL; i++)
    {
        for (int j = 0; j < SIZE_NAVBALL; j++)
        {
            g_px_hx[i][j] = -1.0 + (float)j * STEP + 1.0 / (float)SIZE_NAVBALL;
            g_px_hy[i][j] = -1.0 + (float)i * STEP + 1.0 / (float)SIZE_NAVBALL; 
            r2 = g_px_hx[i][j] * g_px_hx[i][j] + g_px_hy[i][j] * g_px_hy[i][j]; 
            if(r2 <= 1.0)
            {
                g_px_hz[i][j] = -sqrt(1.0 - r2);
                g_r2[i][j] = 1;
            }
            else
            {
                g_px_hz[i][j] = NAN;
                g_r2[i][j] = 0;
            }
        }
    }
    init_g_atan2();
    init_g_asin();
    // init the sin and cos table
    for (int i = 0; i < SIZE_PRECOMPUTED_VALUES_SIN_COS; i++)
    {
        // we just need the value between 0 and pi because pi to 2pi is the same as 0 to pi with inverted sign
        g_sin[i] = sin((float)i * M_PI / (float)SIZE_PRECOMPUTED_VALUES_SIN_COS);
        g_cos[i] = cos((float)i * M_PI / (float)SIZE_PRECOMPUTED_VALUES_SIN_COS);
    }
}




void generateNavBall(textureMap_t *texture, navballImage_t *navballImage, float pitch, float roll, float yaw)
{
    float temp_hx, temp_hy, temp_hz;
    double cs, ss, ct, st, cy, sy = 0.0;
    uint8_t *texture_data;
    uint32_t index_texture;
    int x, y;

    // real sin and cos :
    // cs = cos(roll);
    // ct = cos(pitch);
    // cy = cos(yaw);
    // ss = sin(roll);
    // st = sin(pitch);
    // sy = sin(yaw);
    cs = fastCos(roll);
    ct = fastCos(pitch);
    cy = fastCos(yaw);
    ss = fastSin(roll);
    st = fastSin(pitch);
    sy = fastSin(yaw);

    for (int i = 0; i < SIZE_NAVBALL; i++)
    {
        for (int j = 0; j < SIZE_NAVBALL; j++)
        {
            // if(g_r2[i][j] <= 1.0)
            if(g_r2[i][j])
            {
                temp_hx = cs * cy * g_px_hx[i][j] + cs * sy * g_px_hy[i][j] + ss * g_px_hz[i][j];
                temp_hy = (st * -ss * cy + ct * -sy) * g_px_hx[i][j] + (st * -ss * sy + ct * cy) * g_px_hy[i][j] + st * cs * g_px_hz[i][j];
                temp_hz = (ct * -ss * cy + -st * -sy) * g_px_hx[i][j] + (ct * -ss * sy + -st * cy) * g_px_hy[i][j] + ct * cs * g_px_hz[i][j];
                // x = (int)(DEMI_TEXTURE_MAP_HEIGHT + (asin(temp_hy) * TEXTURE_MAP_HEIGHT_PI)); // optimised correct
                x = (int)(DEMI_TEXTURE_MAP_HEIGHT + fastAsin(temp_hy) * TEXTURE_MAP_HEIGHT_PI); // optimised correct
                
                // y = (int)((DEMI_TEXTURE_MAP_WIDTH + atan2(temp_hz, temp_hx) * DEMI_TEXTURE_MAP_WIDTH_PI)); // optimised correct
                y = (int)((DEMI_TEXTURE_MAP_WIDTH + fastAtan2(temp_hz, temp_hx) * DEMI_TEXTURE_MAP_WIDTH_PI)); // optimised correct
                index_texture = x * TEXTURE_MAP_WIDTH * 3 + y * 3;
                //check if the index is in the range of the texture this shit is needed only when using the appoximated atan2 function
                if(index_texture >= TEXTURE_MAP_SIZE -1) {index_texture = 0;}
                texture_data = &texture->data[index_texture];
                navballImage->data[i][j][0] = (int)texture_data[0];
                navballImage->data[i][j][1] = (int)texture_data[1];
                navballImage->data[i][j][2] = (int)texture_data[2];

            }
            else
            {
                // navballImage->data[i][j][0] = 0;
                // navballImage->data[i][j][1] = 0;
                // navballImage->data[i][j][2] = 0;
            }
        }
    }
}

void unit_test_trigo(double eps)
{
    double x, y;
    double res, ref;
    for (int i = 0; i < 1000; i++)
    {
        // for atan2 we need value between RANGE_ATAN2 and -RANGE_ATAN2
        x = (double)rand() / RAND_MAX * 2 * RANGE_ATAN2 - RANGE_ATAN2;
        y = (double)rand() / RAND_MAX * 2 * RANGE_ATAN2 - RANGE_ATAN2;
        res = fastAtan2(x, y);
        ref = atan2(x, y);
        if (fabs(res - ref) > eps)
        {
            printf("Error atan2 %f %f %f %f\n", x, y, res, ref);
        }
        // for asin we need value between RANGE_ASIN and -RANGE_ASIN
        x = (double)rand() / RAND_MAX * 2 * RANGE_ASIN - RANGE_ASIN;
        res = fastAsin(x);
        ref = asin(x);
        if (fabs(res - ref) > eps)
        {
            printf("Error asin %f %f\n", res, ref);
        }
        // test between -2pi and 2pi
        x = (double)rand() / RAND_MAX * 4 * M_PI - 2 * M_PI;
        res = fastSin(x);
        ref = sin(x);
        if (fabs(res - ref) > eps)
        {
            printf("Error sin %f %f\n", res, ref);
        }
        res = fastCos(x);
        ref = cos(x);
        if (fabs(res - ref) > eps)
        {
            printf("Error cos %f %f\n", res, ref);
        }
    }
}

void generateNavBallMt(textureMap_t *texture, navballImage_t *navballImage, float pitch, float roll, float yaw)
{
    double cs, ss, ct, st, cy, sy = 0.0;
    param_navball_thread_t param[NUM_THEARD];
    pthread_t thread[NUM_THEARD];
    cs = cos(roll);
    ct = cos(pitch);
    cy = cos(yaw);
    ss = sin(roll);
    st = sin(pitch);
    sy = sin(yaw);
    for (int i = 0; i < NUM_THEARD; i++)
    {
        param[i].id = i;
        param[i].texture = texture;
        param[i].navballImage = navballImage;
        param[i].cs = &cs;
        param[i].ss = &ss;
        param[i].ct = &ct;
        param[i].st = &st;
        param[i].cy = &cy;
        param[i].sy = &sy;

        pthread_create(&thread[i], NULL, generateNavBallThread, &param[i]);
    }

    for (int i = 0; i < NUM_THEARD; i++)
    {
        pthread_join(thread[i], NULL);
    }

}

void *generateNavBallThread(void *arg)
{
    param_navball_thread_t *param = (param_navball_thread_t *)arg;
    float temp_hx, temp_hy, temp_hz;
    uint8_t *texture_data;
    int x, y;

    for (int i = param->id; i < SIZE_NAVBALL; i+=NUM_THEARD)
    {
        for (int j = 0; j < SIZE_NAVBALL; j++)
        {
            if(g_r2[i][j] <= 1.0)
            {
                temp_hx = *param->cs * *param->cy * g_px_hx[i][j] + *param->cs * *param->sy * g_px_hy[i][j] + *param->ss * g_px_hz[i][j];
                temp_hy = (*param->st * -(*param->ss) * *param->cy + *param->ct * -(*param->sy) * g_px_hx[i][j] + (*param->st * -(*param->ss) * *param->sy + *param->ct * *param->cy) * g_px_hy[i][j] + *param->st * *param->cs * g_px_hz[i][j]);
                temp_hz = (*param->ct * -(*param->ss) * *param->cy + -(*param->st) * -(*param->sy)) * g_px_hx[i][j] + (*param->ct * -(*param->ss) * *param->sy + -(*param->st) * *param->cy) * g_px_hy[i][j] + *param->ct * *param->cs * g_px_hz[i][j];
                // x = (int)((0.5 + (asin(temp_hy)) / M_PI) * TEXTURE_MAP_HEIGHT); // real formula
                x = (int)(DEMI_TEXTURE_MAP_HEIGHT + (asin(temp_hy) * TEXTURE_MAP_HEIGHT_PI)); // optimised correct
                
                // y = (int)((1.0 + atan2(temp_hz, temp_hx) / M_PI) * 0.5 * TEXTURE_MAP_WIDTH); // real formula
                y = (int)((DEMI_TEXTURE_MAP_WIDTH + atan2(temp_hz, temp_hx) * DEMI_TEXTURE_MAP_WIDTH_PI)); // optimised correct
                texture_data = &param->texture->data[x * TEXTURE_MAP_WIDTH * 3 + y * 3];
                param->navballImage->data[i][j][0] = (int)texture_data[0];
                param->navballImage->data[i][j][1] = (int)texture_data[1];
                param->navballImage->data[i][j][2] = (int)texture_data[2];

            }
            else
            {
                // set the color to black
                // px_hz = NAN;
                param->navballImage->data[i][j][0] = 0;
                param->navballImage->data[i][j][1] = 0;
                param->navballImage->data[i][j][2] = 0;
            }
        }
    }
}


void savePPM(navballImage_t *image, char *filename)
{
    FILE *file = fopen(filename, "w");
    if (file == NULL)
    {
        printf("Error opening file %s \n", filename);
        exit(1);
    }
    fprintf(file, "P6\n");
    fprintf(file, "%d %d\n", SIZE_NAVBALL, SIZE_NAVBALL);
    fprintf(file, "255\n");
    for (int i = 0; i < SIZE_NAVBALL; i++)
    {
        for (int j = 0; j < SIZE_NAVBALL; j++)
        {
            for (int k = 0; k < 3; k++)
            {
                fputc(image->data[i][j][k], file);
            }
        }
    }
    fclose(file);
}