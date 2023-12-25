/*
Fichier : navball.c
Auteur : Tanguy Dietrich
Description :
*/
#include "navball.h"
#include <stdio.h>
#include <stdlib.h>

static float g_px_hx[SIZE_NAVBALL][SIZE_NAVBALL];
static float g_px_hy[SIZE_NAVBALL][SIZE_NAVBALL];
static float g_px_hz[SIZE_NAVBALL][SIZE_NAVBALL];
static float g_r2[SIZE_NAVBALL][SIZE_NAVBALL];

// static int g_px_hx[SIZE_NAVBALL][SIZE_NAVBALL];
// static int g_px_hy[SIZE_NAVBALL][SIZE_NAVBALL];
// static int g_px_hz[SIZE_NAVBALL][SIZE_NAVBALL];
// static int g_r2[SIZE_NAVBALL][SIZE_NAVBALL];

// static int32_t g_px_hx_fp[SIZE_NAVBALL][SIZE_NAVBALL];
// static int32_t g_px_hy_fp[SIZE_NAVBALL][SIZE_NAVBALL];
// static int32_t g_px_hz_fp[SIZE_NAVBALL][SIZE_NAVBALL];

// static int32_t g_r2_fp[SIZE_NAVBALL][SIZE_NAVBALL];

#ifdef ENABLE_PRE_COMPUTED_SIN_COS_VALUES
// static float g_sin[SIZE_PRECOMPUTED_VALUES_SIN_COS];
// static float g_cos[SIZE_PRECOMPUTED_VALUES_SIN_COS];

static int g_sin[SIZE_PRECOMPUTED_VALUES_SIN_COS];
static int g_cos[SIZE_PRECOMPUTED_VALUES_SIN_COS];

#endif

void generateNavBall(textureMap_t *texture, navballImage_t *navballImage, float pitch, float roll, float yaw)
{
    float temp_hx, temp_hy, temp_hz;
    double cs, ss, ct, st, cy, sy = 0.0;
    uint8_t *texture_data;
    int x, y;
    cs = cos(roll);
    ct = cos(pitch);
    cy = cos(yaw);
    ss = sin(roll);
    st = sin(pitch);
    sy = sin(yaw);

    for (int i = 0; i < SIZE_NAVBALL; i++)
    {
        for (int j = 0; j < SIZE_NAVBALL; j++)
        {
            if(g_r2[i][j] <= 1.0)
            {
                temp_hx = cs * cy * g_px_hx[i][j] + cs * sy * g_px_hy[i][j] + ss * g_px_hz[i][j];
                temp_hy = (st * -ss * cy + ct * -sy) * g_px_hx[i][j] + (st * -ss * sy + ct * cy) * g_px_hy[i][j] + st * cs * g_px_hz[i][j];
                temp_hz = (ct * -ss * cy + -st * -sy) * g_px_hx[i][j] + (ct * -ss * sy + -st * cy) * g_px_hy[i][j] + ct * cs * g_px_hz[i][j];
                // x = (int)((0.5 + (asin(temp_hy)) / M_PI) * TEXTURE_MAP_HEIGHT); // real formula
                x = (int)(DEMI_TEXTURE_MAP_HEIGHT + (asin(temp_hy) * TEXTURE_MAP_HEIGHT_PI)); // optimised correct
                
                // y = (int)((1.0 + atan2(temp_hz, temp_hx) / M_PI) * 0.5 * TEXTURE_MAP_WIDTH); // real formula
                y = (int)((DEMI_TEXTURE_MAP_WIDTH + atan2(temp_hz, temp_hx) * DEMI_TEXTURE_MAP_WIDTH_PI)); // optimised correct
                texture_data = &texture->data[x * TEXTURE_MAP_WIDTH * 3 + y * 3];
                navballImage->data[i][j][0] = (int)texture_data[0];
                navballImage->data[i][j][1] = (int)texture_data[1];
                navballImage->data[i][j][2] = (int)texture_data[2];

            }
            else
            {
                // set the color to black
                // px_hz = NAN;
                navballImage->data[i][j][0] = 0;
                navballImage->data[i][j][1] = 0;
                navballImage->data[i][j][2] = 0;
            }
        }
    }
}

void generateNavBallMt(textureMap_t *texture, navballImage_t *navballImage, float pitch, float roll, float yaw)
{
    float temp_hx, temp_hy, temp_hz;
    double cs, ss, ct, st, cy, sy = 0.0;
    uint8_t *texture_data;
    int x, y;
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

    

// void generateNavBallFast(textureMap_t *texture, navballImage_t *navballImage, int pitch, int roll, int yaw)
// {
//     #ifndef ENABLE_PRECOMPUTED_VALUES
//     float r2, px_hx, px_hy, px_hz;
//     #endif
//     int temp_hx, temp_hy, temp_hz;
//     // float temp_hx, temp_hy, temp_hz;
//     // float cs, ss, ct, st, cy, sy = 0.0;
//     int cs, ss, ct, st, cy, sy = 0;
//     // uint8_t r,g,b;
//     int x, y;

//     // printf("pitch: %d, roll: %d, yaw: %d\r\n", pitch, roll, yaw);
//     // roll, pitch, yaw are in PRECOMPUTED VERSION (multiply by CONVERT_RAD_TO_PRE)
//     cs = g_cos[roll];
//     ct = g_cos[pitch];
//     cy = g_cos[yaw];
//     ss = g_sin[roll];
//     st = g_sin[pitch];
//     sy = g_sin[yaw];

//     for (int i = 0; i < SIZE_NAVBALL; i++)
//     {
//         for (int j = 0; j < SIZE_NAVBALL; j++)
//         {
//             if(g_r2[i][j] <= MULTIPLIER * MULTIPLIER) 
//             {
//                 temp_hx = cs * cy * g_px_hx[i][j] + cs * sy * g_px_hy[i][j] + ss * g_px_hz[i][j];
//                 temp_hy = (st * -ss * cy + ct * -sy) * g_px_hx[i][j] + (st * -ss * sy + ct * cy) * g_px_hy[i][j] + st * cs * g_px_hz[i][j];
//                 temp_hz = (ct * -ss * cy + -st * -sy) * g_px_hx[i][j] + (ct * -ss * sy + -st * cy) * g_px_hy[i][j] + ct * cs * g_px_hz[i][j];
//                 x = (int)((0.5 + (asin(temp_hy)) / M_PI) * TEXTURE_MAP_HEIGHT) / MULTIPLIER;
//                 // x = (int)((0.5 + (FastASin(temp_hy)) / M_PI) * TEXTURE_MAP_HEIGHT);
//                 y = (int)((1.0 + atan2(temp_hz, temp_hx) / M_PI) * 0.5 * TEXTURE_MAP_WIDTH) / MULTIPLIER;
//                 // y = (int)((1.0 + FastArcTan2(temp_hz, temp_hx) / M_PI) * 0.5 * TEXTURE_MAP_WIDTH);
//                 printf("x: %d, y: %d\r\n", x, y);
//                 navballImage->data[i][j][0] = (int) texture->data[x * TEXTURE_MAP_WIDTH * 3 + y * 3 + 0];
//                 navballImage->data[i][j][1] = (int) texture->data[x * TEXTURE_MAP_WIDTH * 3 + y * 3 + 1];
//                 navballImage->data[i][j][2] = (int) texture->data[x * TEXTURE_MAP_WIDTH * 3 + y * 3 + 2];
//             }
//             else
//             {
//                 // set the color to black
//                 // px_hz = NAN;
//                 navballImage->data[i][j][0] = 0;
//                 navballImage->data[i][j][1] = 0;
//                 navballImage->data[i][j][2] = 0;
//             }
//         }
//     }
// }


void initPreComputedValue()
{
    for (int i = 0; i < SIZE_NAVBALL; i++)
    {
        for (int j = 0; j < SIZE_NAVBALL; j++)
        {
            g_px_hx[i][j] = -1.0 + (float)j * STEP + 1.0 / (float)SIZE_NAVBALL;
            g_px_hy[i][j] = -1.0 + (float)i * STEP + 1.0 / (float)SIZE_NAVBALL; 
            g_r2[i][j] = g_px_hx[i][j] * g_px_hx[i][j] + g_px_hy[i][j] * g_px_hy[i][j]; 
            if(g_r2[i][j] <= 1.0)
            {
                g_px_hz[i][j] = -sqrt(1.0 - g_r2[i][j]);
            }
            else
            {
                g_px_hz[i][j] = NAN;
            }
        }
    }
}


// void initPreComputedValue()
// {
//     float px_hx, px_hy, px_hz, r2;
//     for (int i = 0; i < SIZE_NAVBALL; i++)
//     {
//         for (int j = 0; j < SIZE_NAVBALL; j++)
//         {
//             px_hx = -1.0 + (float)j * STEP + 1.0 / (float)SIZE_NAVBALL;
//             px_hy = -1.0 + (float)i * STEP + 1.0 / (float)SIZE_NAVBALL;
//             g_px_hx[i][j] = px_hx * MULTIPLIER;
//             g_px_hy[i][j] = px_hy * MULTIPLIER;
//             r2 = px_hx * px_hx + px_hy * px_hy;
//             g_r2[i][j] = g_px_hx[i][j] * g_px_hx[i][j] + g_px_hy[i][j] * g_px_hy[i][j];
//             // printf("g_px_hx[%d][%d]: %d\r\n", i, j, g_px_hx[i][j]);
//             // printf("g_px_hy[%d][%d]: %d\r\n", i, j, g_px_hy[i][j]);
//             // printf("g_r2[%d][%d]: %d\r\n", i, j, g_r2[i][j]);
//             if(g_r2[i][j] <= MULTIPLIER * MULTIPLIER)
//             {
//                 px_hz = -sqrt(1.0 - r2); 
//                 g_px_hz[i][j] = px_hz * MULTIPLIER;
//             }
//             else
//             {
//                 g_px_hz[i][j] = NAN;
//             }
//         }
//     }
//     // init the precomputed sin and cos
//     float step = M_PI / (float)SIZE_PRECOMPUTED_VALUES_SIN_COS; 
//     for (int i = 0; i < SIZE_PRECOMPUTED_VALUES_SIN_COS; i++)
//     {
//         g_sin[i] = sin(step * (float)i) * MULTIPLIER;
//         g_cos[i] = cos(step * (float)i) * MULTIPLIER;
//     }
// }


// https://stackoverflow.com/questions/11261170/c-and-maths-fast-approximation-of-a-trigonometric-function
double FastASin(double x)
{
  double x8, x4, x2;
  x2 = x * x;
  x4 = x2 * x2;
  x8 = x4 * x4;
  /* evaluate polynomial using a mix of Estrin's and Horner's scheme */
  return (((4.5334220547132049e-2 * x2 - 1.1226216762576600e-2) * x4 +
           (2.6334281471361822e-2 * x2 + 2.0596336163223834e-2)) * x8 +
          (3.0582043602875735e-2 * x2 + 4.4630538556294605e-2) * x4 +
          (7.5000364034134126e-2 * x2 + 1.6666666300567365e-1)) * x2 * x + x; 
}



/* minimax approximation to cos on [-pi/4, pi/4] with rel. err. ~= 7.5e-13 */
double FastCos (double x)
{
  double x8, x4, x2;
  x2 = x * x;
  x4 = x2 * x2;
  x8 = x4 * x4;
  /* evaluate polynomial using Estrin's scheme */
  return (-2.7236370439787708e-7 * x2 + 2.4799852696610628e-5) * x8 +
         (-1.3888885054799695e-3 * x2 + 4.1666666636943683e-2) * x4 +
         (-4.9999999999963024e-1 * x2 + 1.0000000000000000e+0);
}



/* minimax approximation to sin on [-pi/4, pi/4] with rel. err. ~= 5.5e-12 */
double FastSin (double x)
{
  double x4, x2;
  x2 = x * x;
  x4 = x2 * x2;
  /* evaluate polynomial using a mix of Estrin's and Horner's scheme */
  return ((2.7181216275479732e-6 * x2 - 1.9839312269456257e-4) * x4 + 
          (8.3333293048425631e-3 * x2 - 1.6666666640797048e-1)) * x2 * x + x;
}



// // https://developer.download.nvidia.com/cg/asin.html
// double FastASin(double x)
// {
//     double negate = (double)(x < 0);
//     x = fabs(x);
//     double ret = -0.0187293;
//     ret *= x;
//     ret += 0.0742610;
//     ret *= x;
//     ret -= 0.2121144;
//     ret *= x;
//     ret += 1.5707288;
//     ret = 3.14159265358979*0.5 - sqrt(1.0 - x)*ret;
//     return ret - 2 * negate * ret;
// }

double FastArcTan(double x)
{
	return M_PI_4 * x - x * (fabs(x) - 1) * (0.2447 + 0.0663 * fabs(x));
}




// https://yal.cc/fast-atan2/
double FastArcTan2(double y, double x)
{
	if (x >= 0) { // -pi/2 .. pi/2
		if (y >= 0) { // 0 .. pi/2
			if (y < x) { // 0 .. pi/4
				return FastArcTan(y / x);
			} else { // pi/4 .. pi/2
				return M_PI_2 - FastArcTan(x / y);
			}
		} else {
			if (-y < x) { // -pi/4 .. 0
				return FastArcTan(y / x);
			} else { // -pi/2 .. -pi/4
				return -M_PI_2 - FastArcTan(x / y);
			}
		}
	} else { // -pi..-pi/2, pi/2..pi
		if (y >= 0) { // pi/2 .. pi
			if (y < -x) { // pi*3/4 .. pi
				return FastArcTan(y / x) + M_PI;
			} else { // pi/2 .. pi*3/4
				return M_PI_2 - FastArcTan(x / y);
			}
		} else { // -pi .. -pi/2
			if (-y < -x) { // -pi .. -pi*3/4
				return FastArcTan(y / x) - M_PI;
			} else { // -pi*3/4 .. -pi/2
				return -M_PI_2 - FastArcTan(x / y);
			}
		}
	}
}



// AA THESE FUNCTIONS ARE NOT USED ANYMORE
void bilinear(float x, float y, textureMap_t *texture, uint8_t *r, uint8_t *g, uint8_t *b)
{
    int i,j,k,l;
    int x1 = floor(x);
    int x2 = ceil(x);
    int y1 = floor(y);
    int y2 = ceil(y);
    float x1y1 = (x2 - x) * (y2 - y);
    float x2y1 = (x - x1) * (y2 - y);
    float x1y2 = (x2 - x) * (y - y1);
    float x2y2 = (x - x1) * (y - y1);
    i = x1 * TEXTURE_MAP_WIDTH * 3 + y1 * 3 + 0;
    j = x2 * TEXTURE_MAP_WIDTH * 3 + y1 * 3 + 0;
    k = x1 * TEXTURE_MAP_WIDTH * 3 + y2 * 3 + 0;
    l = x2 * TEXTURE_MAP_WIDTH * 3 + y2 * 3 + 0;
    // *r = x1y1 * texture->data[x1][y1][0] + x2y1 * texture->data[x2][y1][0] + x1y2 * texture->data[x1][y2][0] + x2y2 * texture->data[x2][y2][0];
    // *g = x1y1 * texture->data[x1][y1][1] + x2y1 * texture->data[x2][y1][1] + x1y2 * texture->data[x1][y2][1] + x2y2 * texture->data[x2][y2][1];
    // *b = x1y1 * texture->data[x1][y1][2] + x2y1 * texture->data[x2][y1][2] + x1y2 * texture->data[x1][y2][2] + x2y2 * texture->data[x2][y2][2];

    *r = x1y1 * texture->data[i] + x2y1 * texture->data[j] + x1y2 * texture->data[k] + x2y2 * texture->data[l];
    *g = x1y1 * texture->data[i] + x2y1 * texture->data[j] + x1y2 * texture->data[k] + x2y2 * texture->data[l];
    *b = x1y1 * texture->data[i] + x2y1 * texture->data[j] + x1y2 * texture->data[k] + x2y2 * texture->data[l];
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