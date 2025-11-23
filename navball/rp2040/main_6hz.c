#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>

#include <pico/stdlib.h> 
#include <hardware/gpio.h>
#include <pico/multicore.h>


// #include <hardware/spi.h>
#include <pico/time.h>


/* ===== Config ===== */
#ifndef NAVBALL_MAX_SIZE
#define NAVBALL_MAX_SIZE  480
#endif

#define Q15_ONE   32767
#define CLAMP(v,a,b) ((v)<(a)?(a):((v)>(b)?(b):(v)))

/* ===== Colors (8-bit channels) ===== */
static const uint8_t SKY_ZENITH[3]   = {  83, 155, 245};
static const uint8_t SKY_HORIZON[3]  = {  33, 104, 214};
static const uint8_t GND_HORIZON[3]  = { 172, 108,  58};
static const uint8_t GND_NADIR[3]    = { 112,  70,  35};
static const uint8_t LINE_RGB[3]     = { 245, 245, 245};

/* ===== Fixed-point helpers ===== */
static inline int32_t q15_mul(int32_t a, int32_t b) {
    int64_t t = (int64_t)a * (int64_t)b;
    t += (1 << 14);
    t >>= 15;
    if (t >  32767) t =  32767;
    if (t < -32767) t = -32767;
    return (int32_t)t;
}
static inline int fast_hypot_q15(int32_t x, int32_t y) {
    int32_t ax = x < 0 ? -x : x;
    int32_t ay = y < 0 ? -y : y;
    int32_t mx = ax > ay ? ax : ay;
    int32_t mn = ax > ay ? ay : ax;
    int32_t h  = mx + (mn >> 1);
    return h > Q15_ONE ? Q15_ONE : h;
}
static inline uint16_t pack_rgb565(uint8_t r, uint8_t g, uint8_t b) {
    return (uint16_t)(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
}
static inline uint16_t lerp_rgb565(const uint8_t a[3], const uint8_t b[3], uint16_t t /*0..255*/) {
    uint16_t r = ( ( (uint16_t)a[0]*(255 - t) + (uint16_t)b[0]*t ) / 255 );
    uint16_t g = ( ( (uint16_t)a[1]*(255 - t) + (uint16_t)b[1]*t ) / 255 );
    uint16_t bb= ( ( (uint16_t)a[2]*(255 - t) + (uint16_t)b[2]*t ) / 255 );
    return pack_rgb565((uint8_t)r, (uint8_t)g, (uint8_t)bb);
}

/* ===== Triangle fill (12 o’clock marker) ===== */
static void fill_tri_rgb565(uint16_t* fb, int W, int H, int stride,
                            int x0,int y0,int x1,int y1,int x2,int y2,
                            uint16_t color)
{
    int minx = x0; if (x1 < minx) minx = x1; if (x2 < minx) minx = x2;
    int maxx = x0; if (x1 > maxx) maxx = x1; if (x2 > maxx) maxx = x2;
    int miny = y0; if (y1 < miny) miny = y1; if (y2 < miny) miny = y2;
    int maxy = y0; if (y1 > maxy) maxy = y1; if (y2 > maxy) maxy = y2;

    if (minx < 0) minx = 0; if (miny < 0) miny = 0;
    if (maxx >= W) maxx = W-1; if (maxy >= H) maxy = H-1;

    int A = (x1-x0)*(y2-y0) - (x2-x0)*(y1-y0);
    if (A == 0) return;

    for (int y = miny; y <= maxy; ++y) {
        for (int x = minx; x <= maxx; ++x) {
            int w0 = (x1-x0)*(y-y0) - (y1-y0)*(x-x0);
            int w1 = (x2-x1)*(y-y1) - (y2-y1)*(x-x1);
            int w2 = (x0-x2)*(y-y2) - (y0-y2)*(x-x2);
            if ((w0|w1|w2) >= 0 || (w0|w1|w2) == 0) {
                fb[y*stride + x] = color;
            }
        }
    }
}

/* ===== LUTs built once (soft-float at init; NOT per pixel) ===== */
static int16_t SIN_DEG[360], COS_DEG[360];
static int16_t SQRT_1_MINUS[1025];

static void init_luts(void) {
    static int inited = 0;
    if (inited) return;
    inited = 1;

    for (int d = 0; d < 360; ++d) {
        double rad = (double)d * (M_PI/180.0);
        long s = lround(sin(rad) * (double)Q15_ONE);
        long c = lround(cos(rad) * (double)Q15_ONE);
        if (s >  Q15_ONE) s =  Q15_ONE; if (s < -Q15_ONE) s = -Q15_ONE;
        if (c >  Q15_ONE) c =  Q15_ONE; if (c < -Q15_ONE) c = -Q15_ONE;
        SIN_DEG[d] = (int16_t)s;
        COS_DEG[d] = (int16_t)c;
    }
    for (int i = 0; i <= 1024; ++i) {
        double t = (double)i / 1024.0;
        double z = sqrt(fmax(0.0, 1.0 - t));
        long  q = lround(z * (double)Q15_ONE);
        if (q < 0) q = 0; if (q > Q15_ONE) q = Q15_ONE;
        SQRT_1_MINUS[i] = (int16_t)q;
    }
}

/* ===== Euler to matrix (Q15), R = Rz*Ry*Rx ===== */
static void euler_to_R_q15(int yaw_deg, int pitch_deg, int roll_deg, int32_t R[9]) {
    int y = ((yaw_deg%360)+360)%360;
    int p = ((pitch_deg%360)+360)%360;
    int r = ((roll_deg%360)+360)%360;

    int32_t cy = COS_DEG[y],  sy = SIN_DEG[y];
    int32_t cp = COS_DEG[p],  sp = SIN_DEG[p];
    int32_t cr = COS_DEG[r],  sr = SIN_DEG[r];

    int32_t Rz[9] = { cy, -sy,    0,
                      sy,  cy,    0,
                       0,   0, Q15_ONE };
    int32_t Ry[9] = { cp,    0,  sp,
                       0, Q15_ONE, 0,
                     -sp,    0,  cp };
    int32_t Rx[9] = { Q15_ONE, 0,   0,
                       0,    cr, -sr,
                       0,    sr,  cr };

    int32_t T[9];
    for (int i=0;i<3;i++){
        for (int j=0;j<3;j++){
            int idx = 3*i + j;
            int32_t s0 = q15_mul(Rz[3*i+0], Ry[0*3+j]);
            int32_t s1 = q15_mul(Rz[3*i+1], Ry[1*3+j]);
            int32_t s2 = q15_mul(Rz[3*i+2], Ry[2*3+j]);
            T[idx] = CLAMP(s0 + s1 + s2, -Q15_ONE, Q15_ONE);
        }
    }
    for (int i=0;i<3;i++){
        for (int j=0;j<3;j++){
            int idx = 3*i + j;
            int32_t s0 = q15_mul(T[3*i+0], Rx[0*3+j]);
            int32_t s1 = q15_mul(T[3*i+1], Rx[1*3+j]);
            int32_t s2 = q15_mul(T[3*i+2], Rx[2*3+j]);
            R[idx] = CLAMP(s0 + s1 + s2, -Q15_ONE, Q15_ONE);
        }
    }
}

/* ===== Main renderer =====
   NOTE: “Up” is +Y. We classify by vy; longitudes are around Y using XZ plane.
*/
void navball_render_rgb565(
    uint16_t* fb, int W, int H, int stride,
    int yaw_deg, int pitch_deg, int roll_deg, int draw_grid)
{

    int D = (W < H ? W : H);
    if (D > NAVBALL_MAX_SIZE) D = NAVBALL_MAX_SIZE;
    int cx = W/2, cy = H/2;
    int radius_px = (D - 2) / 2;
    if (radius_px < 24) return;

    /* thickness thresholds (Q15) ≈ px / radius */
    int32_t eps_grid = CLAMP((Q15_ONE + radius_px/2) / radius_px, 200, 3000);
    int32_t eps_hor  = CLAMP((2*Q15_ONE + radius_px/2) / radius_px, 400, 6000);

    int32_t R[9]; euler_to_R_q15(yaw_deg, pitch_deg, roll_deg, R);
    int32_t RT[9] = { R[0],R[3],R[6], R[1],R[4],R[7], R[2],R[5],R[8] };

    static int32_t xQ[NAVBALL_MAX_SIZE], yQ[NAVBALL_MAX_SIZE];
    for (int x = cx - radius_px; x <= cx + radius_px; ++x) {
        int idx = x - (cx - radius_px);
        int32_t q = (int32_t)(( (x - cx) * Q15_ONE ) / (radius_px));
        xQ[idx] = q;
    }
    for (int y = cy - radius_px; y <= cy + radius_px; ++y) {
        int idy = y - (cy - radius_px);
        int32_t q = (int32_t)(( (cy - y) * Q15_ONE ) / (radius_px)); /* +Y up */
        yQ[idy] = q;
    }

    /* Latitudes around +Y: we compare vy to sin(lat) */
    const int lat_deg_list[] = {-60,-30,30,60};
    int32_t lat_sin[4];
    for (int i=0;i<4;i++){
        int a = ((lat_deg_list[i]%360)+360)%360;
        lat_sin[i] = SIN_DEG[a];
    }
    /* Longitudes around +Y: angles m in XZ plane (φ = atan2(vx, vz)) */
    int32_t lon_sin[12], lon_cos[12];
    for (int i=0;i<12;i++){
        int a = (i*30)%360;
        lon_sin[i] = SIN_DEG[a]; /* sin(m) */
        lon_cos[i] = COS_DEG[a]; /* cos(m) */
    }

    for (int iy = cy - radius_px; iy <= cy + radius_px; ++iy) {
        int idy = iy - (cy - radius_px);
        int32_t yq = yQ[idy];
        uint16_t* row = fb + iy*stride;

        for (int ix = cx - radius_px; ix <= cx + radius_px; ++ix) {
            int idx = ix - (cx - radius_px);
            int32_t xq = xQ[idx];

            /* inside disk? */
            int64_t x2 = (int64_t)xq * xq;
            int64_t y2 = (int64_t)yq * yq;
            int64_t r2q30 = x2 + y2;
            if (r2q30 > ((int64_t)Q15_ONE*Q15_ONE)) {
                row[ix] = 0;
                continue;
            }

            int r2_idx = (int)((r2q30 >> 15) * 1024 / Q15_ONE);
            if (r2_idx > 1024) r2_idx = 1024;
            int32_t zq = SQRT_1_MINUS[r2_idx];

            /* v_nav = R^T * v_cam; v_cam=(xq,yq,zq) with +Y up */
            int32_t vx = CLAMP(q15_mul(RT[0], xq) + q15_mul(RT[1], yq) + q15_mul(RT[2], zq), -Q15_ONE, Q15_ONE);
            int32_t vy = CLAMP(q15_mul(RT[3], xq) + q15_mul(RT[4], yq) + q15_mul(RT[5], zq), -Q15_ONE, Q15_ONE);
            int32_t vz = CLAMP(q15_mul(RT[6], xq) + q15_mul(RT[7], yq) + q15_mul(RT[8], zq), -Q15_ONE, Q15_ONE);

            /* base color by |vy| (latitude around +Y) */
            uint16_t base;
            uint16_t t8 = (uint16_t)(((vy<0?-vy:vy) * 255) / Q15_ONE);
            if (vy >= 0) base = lerp_rgb565(SKY_HORIZON, SKY_ZENITH, t8);
            else         base = lerp_rgb565(GND_HORIZON, GND_NADIR, t8);

            if (draw_grid) {
                int on_line = 0;

                /* horizon band: |vy| < eps_hor */
                if ((vy < 0 ? -vy : vy) < eps_hor) on_line = 1;

                /* latitude lines: |vy - sin(lat)| < eps_grid */
                if (!on_line) {
                    for (int k=0;k<4;k++){
                        int32_t d = vy - lat_sin[k];
                        if (d < 0) d = -d;
                        if (d < eps_grid) { on_line = 1; break; }
                    }
                }

                /* longitude lines around Y: φ = atan2(vx, vz) without atan2
                   distance to meridian m: |sin(φ - m)| ≈ |vx*cos m - vz*sin m| / hypot(vx,vz) */
                if (!on_line) {
                    int32_t hypot_xz = fast_hypot_q15(vx, vz);
                    int32_t thr = (int32_t)(( (int64_t)eps_grid * hypot_xz ) >> 15);
                    for (int m=0;m<12;m++){
                        int32_t d = q15_mul(vx, lon_cos[m]) - q15_mul(vz, lon_sin[m]);
                        if (d < 0) d = -d;
                        if (d < thr) { on_line = 1; break; }
                    }
                }

                if (on_line) {
                    uint8_t br = LINE_RGB[0], bg = LINE_RGB[1], bb = LINE_RGB[2];
                    uint8_t r = ((base >> 11) & 0x1F) << 3;
                    uint8_t g = ((base >> 5)  & 0x3F) << 2;
                    uint8_t b = ( base        & 0x1F) << 3;
                    r = (uint8_t)((r*60 + br*40)/100);
                    g = (uint8_t)((g*60 + bg*40)/100);
                    b = (uint8_t)((b*60 + bb*40)/100);
                    base = pack_rgb565(r,g,b);
                }
            }

            row[ix] = base;
        }
    }

    /* rim (1–2 px) */
    int r_out = radius_px;
    int r_in  = radius_px - 2;
    uint16_t white = pack_rgb565(255,255,255);
    for (int y = cy - radius_px; y <= cy + radius_px; ++y) {
        uint16_t* row = fb + y*stride;
        for (int x = cx - radius_px; x <= cx + radius_px; ++x) {
            int dx = x - cx, dy = y - cy;
            int r2 = dx*dx + dy*dy;
            if (r2 <= r_out*r_out && r2 >= r_in*r_in)
                row[x] = white;
        }
    }

    /* 12 o'clock marker */
    int tip_x = cx, tip_y = cy - radius_px + 6;
    int left_x = cx - 8, left_y = cy - radius_px + 14;
    int right_x= cx + 8, right_y= cy - radius_px + 14;
    fill_tri_rgb565(fb, W,H,stride, tip_x,tip_y, left_x,left_y, right_x,right_y, white);
}

/**********************************************************************/
/*-------------------------- PROTOTYPES ------------------------------*/
/**********************************************************************/
void core_task1(void);

#define WIDTH 120
#define HEIGHT 120
#define STRIDE WIDTH
#define ENABLE_GRID 1

/**********************************************************************/
/*--------------------------- MAIN FUNCTION -------------------------*/
/**********************************************************************/


int main() {
    stdio_init_all();

    // Optional: wait for a terminal to open the USB CDC port
    // Comment out if you don't want to block on startup.
    // while (!stdio_usb_connected()) {
    //     sleep_ms(10);
    // }

    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);

    static uint16_t fb[HEIGHT * WIDTH];
    uint16_t yaw = 0, pitch = 0, roll = 0;
    init_luts(); // Called once at startup

    while (1) {
        uint64_t t0 = time_us_64();

        gpio_put(PICO_DEFAULT_LED_PIN, 0);
        navball_render_rgb565(fb, WIDTH, HEIGHT, STRIDE, yaw, pitch, roll, ENABLE_GRID);
        gpio_put(PICO_DEFAULT_LED_PIN, 1);

        uint64_t dt_us = time_us_64() - t0;

        printf("render: %llu us\r\n", (unsigned long long)dt_us);

        yaw   = (yaw   + 5) % 360;
        pitch = (pitch + 3) % 360;
        roll  = (roll  + 7) % 360;
    }

    return 0;
}


/**********************************************************************/
/*------------------------ CORE 1 FUNCTION --------------------------*/
/**********************************************************************/
void core_task1(void)
{
    while (1)
    {
    }
}