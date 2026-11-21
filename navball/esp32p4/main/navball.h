#pragma once
#include <stdint.h>

/* Navball renderer (fixed-point Q15), ported from navball/rp2040/main.c.
 * The frame is prepared once per attitude with navball_prepare(), then any
 * band of rows can be rendered independently (one band per CPU core). */

#define NAVBALL_MAX_SIZE 800

typedef struct {
    uint16_t *fb;
    int W, H, stride;
    int cx, cy, radius_px;
    int xBase, yBase;
    int draw_grid;
    int32_t eps_grid, eps_hor;
    int32_t RT[9];
    int32_t lat_sin[4];
    int32_t lon_sin[12], lon_cos[12];
} navball_ctx_t;

void navball_init(void);   /* LUTs + colour ramps, call once */
/* KSP attitude (degrees) -> M, camera -> navball frame (X east, Y up, Z north) */
void navball_attitude_matrix(float heading, float pitch, float roll, float M[9]);
/* unit vector of a heading / pitch direction (degrees) in the navball frame */
void navball_direction(float heading, float pitch, float d[3]);
void navball_prepare(navball_ctx_t *c, uint16_t *fb, int W, int H, int stride,
                     const float M[9], int draw_grid);
void navball_render_rows(const navball_ctx_t *c, int y_start, int y_end);
/* rows [y_start, y_end] into dst (stride dst_stride); clear: write full rows, black outside the ball */
int navball_simd_selftest(const navball_ctx_t *c);   /* 0 = SIMD kernel matches its C reference */
void navball_render_rows_to(const navball_ctx_t *c, int y_start, int y_end, uint16_t *dst, int dst_stride, int clear);
