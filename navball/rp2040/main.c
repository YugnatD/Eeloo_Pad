#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>

#include <pico/stdlib.h> 
#include <hardware/gpio.h>
#include <pico/multicore.h>

#include "hardware/dma.h"


#include "hardware/spi.h"
#include "hardware/gpio.h"


// #include <hardware/spi.h>
#include <pico/time.h>


#define LCD_WIDTH   240
#define LCD_HEIGHT  240

#define IMAGE_RATIO 1 // 1 = no scaling, 2 = half-size, etc.

#define WIDTH (LCD_WIDTH / IMAGE_RATIO)
#define HEIGHT (LCD_HEIGHT / IMAGE_RATIO)
#define STRIDE WIDTH
#define ENABLE_GRID 1

#define PIN_LCD_SCK   10
#define PIN_LCD_MOSI  11
#define PIN_LCD_MISO  12
#define PIN_LCD_CS     9

#define PIN_LCD_DC     8
#define PIN_LCD_RST   13
#define PIN_LCD_BL    25

#define SPI_PORT spi1


#define DELAY_FLAG 0x80

static int lcd_dma_chan;

// Global RGB565 framebuffer
static uint16_t fb1[WIDTH * HEIGHT];
static uint16_t fb2[WIDTH * HEIGHT];

static uint16_t* fb_to_show = fb1;
static uint16_t* fb_to_draw = fb2;

static volatile bool lcd_frame_done = false;


// --- Low-level LCD helpers (GC9A01A) ---
static inline void cs_select()   { gpio_put(PIN_LCD_CS, 0); }
static inline void cs_deselect() { gpio_put(PIN_LCD_CS, 1); }
static inline void dc_command()  { gpio_put(PIN_LCD_DC, 0); }
static inline void dc_data()     { gpio_put(PIN_LCD_DC, 1); }

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

static const uint8_t gc9a01_init[] = {
    0xFE, 0x00,                          // Inter Register Enable1
    0xEF, 0x00,                          // Inter Register Enable2
    0xB6, 0x02, 0x00, 0x00,              // Display Function Control
    0x36, 0x01, 0x48,                    // MADCTL
    0x3A, 0x01, 0x05,                    // COLMOD: 16bpp
    0xC3, 0x01, 0x13,                    // Power Control 2
    0xC4, 0x01, 0x13,                    // Power Control 3
    0xC9, 0x01, 0x22,                    // Power Control 4
    0xF0, 0x06, 0x45, 0x09, 0x08, 0x08, 0x26, 0x2A,  // SET_GAMMA1
    0xF1, 0x06, 0x43, 0x70, 0x72, 0x36, 0x37, 0x6F,  // SET_GAMMA2
    0xF2, 0x06, 0x45, 0x09, 0x08, 0x08, 0x26, 0x2A,  // SET_GAMMA3
    0xF3, 0x06, 0x43, 0x70, 0x72, 0x36, 0x37, 0x6F,  // SET_GAMMA4
    0x66, 0x0A, 0x3C, 0x00, 0xCD, 0x67, 0x45, 0x45, 0x10, 0x00, 0x00, 0x00,
    0x67, 0x0A, 0x00, 0x3C, 0x00, 0x00, 0x00, 0x01, 0x54, 0x10, 0x32, 0x98,
    0x74, 0x07, 0x10, 0x85, 0x80, 0x00, 0x00, 0x4E, 0x00,
    0x98, 0x02, 0x3E, 0x07,
    0x35, 0x00,                          // TE ON (no args)
    0x21, 0x00,                          // INVON
    0x11, 0x80, 0x78,                    // SLPOUT + delay 120ms
    0x29, 0x80, 0x14,                    // DISPON + delay 20ms
    0x2A, 0x04, 0x00, 0x00, 0x00, 0xEF,  // CASET 0..239
    0x2B, 0x04, 0x00, 0x00, 0x00, 0xEF   // RASET 0..239
};

/* --- Optional: tiny 256-entry RGB565 ramps to avoid per-pixel lerp --- */
static uint16_t SKY_RAMP[256], GND_RAMP[256];




/* ===== Fixed-point helpers ===== */
static inline int32_t q15_mul(int32_t a, int32_t b) {
    // a,b in [-32767,32767] => product fits signed 31-bit
    return (int32_t)((a * b + (1 << 14)) >> 15);
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

static void build_color_ramps(void) {
    for (int t=0; t<256; ++t) {
        uint16_t r, g, b;
        // sky: SKY_HORIZON -> SKY_ZENITH
        r = (( (uint16_t)SKY_HORIZON[0]*(255 - t) + (uint16_t)SKY_ZENITH[0]*t ) / 255);
        g = (( (uint16_t)SKY_HORIZON[1]*(255 - t) + (uint16_t)SKY_ZENITH[1]*t ) / 255);
        b = (( (uint16_t)SKY_HORIZON[2]*(255 - t) + (uint16_t)SKY_ZENITH[2]*t ) / 255);
        SKY_RAMP[t] = pack_rgb565((uint8_t)r,(uint8_t)g,(uint8_t)b);

        // ground: GND_HORIZON -> GND_NADIR
        r = (( (uint16_t)GND_HORIZON[0]*(255 - t) + (uint16_t)GND_NADIR[0]*t ) / 255);
        g = (( (uint16_t)GND_HORIZON[1]*(255 - t) + (uint16_t)GND_NADIR[1]*t ) / 255);
        b = (( (uint16_t)GND_HORIZON[2]*(255 - t) + (uint16_t)GND_NADIR[2]*t ) / 255);
        GND_RAMP[t] = pack_rgb565((uint8_t)r,(uint8_t)g,(uint8_t)b);
    }
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
// fb: pointer to RGB565 framebuffer
// W,H: framebuffer size
// stride: framebuffer row stride in pixels
// yaw_deg, pitch_deg, roll_deg: Euler angles in degrees
void navball_render_rgb565(
    uint16_t* fb, int W, int H, int stride,
    int yaw_deg, int pitch_deg, int roll_deg, int draw_grid)
{    

    int D = (W < H ? W : H);
    if (D > NAVBALL_MAX_SIZE) D = NAVBALL_MAX_SIZE;
    int cx = W/2, cy = H/2;
    int radius_px = (D - 2) / 2;
    if (radius_px < 24) return;

    // thickness thresholds (Q15) ≈ px / radius
    int32_t eps_grid = ((Q15_ONE + radius_px/2) / radius_px);      // ~1 px
    int32_t eps_hor  = ((2*Q15_ONE + radius_px/2) / radius_px);    // ~2 px

    // Rotation (Q15) and transpose
    int32_t R[9]; euler_to_R_q15(yaw_deg, pitch_deg, roll_deg, R);
    int32_t RT[9] = { R[0],R[3],R[6], R[1],R[4],R[7], R[2],R[5],R[8] };

    // Cache x/y normalized arrays; recompute only if radius changes
    static int32_t xQ[NAVBALL_MAX_SIZE], yQ[NAVBALL_MAX_SIZE];
    static int cache_radius = -1, cache_cx = 0, cache_cy = 0, cache_D = 0;
    if (cache_radius != radius_px || cache_cx != cx || cache_cy != cy || cache_D != D) {
        for (int x = cx - radius_px; x <= cx + radius_px; ++x) {
            int idx = x - (cx - radius_px);
            // (x-cx)/radius in Q15
            xQ[idx] = (int32_t)(((int32_t)(x - cx) * Q15_ONE) / radius_px);
        }
        for (int y = cy - radius_px; y <= cy + radius_px; ++y) {
            int idy = y - (cy - radius_px);
            // +Y up: (cy - y)/radius in Q15
            yQ[idy] = (int32_t)(((int32_t)(cy - y) * Q15_ONE) / radius_px);
        }
        cache_radius = radius_px; cache_cx = cx; cache_cy = cy; cache_D = D;
    }

    // Precompute grid constants (lat & long)
    const int lat_deg_list[] = {-60,-30,30,60};
    int32_t lat_sin[4];
    for (int i=0;i<4;i++){
        int a = ((lat_deg_list[i]%360)+360)%360;
        lat_sin[i] = SIN_DEG[a];
    }
    int32_t lon_sin[12], lon_cos[12];
    for (int i=0;i<12;i++){
        int a = (i*30)%360;
        lon_sin[i] = SIN_DEG[a];
        lon_cos[i] = COS_DEG[a];
    }

    const int xBase = cx - radius_px;
    const int yBase = cy - radius_px;
    uint16_t white = pack_rgb565(255,255,255);

    for (int iy = cy - radius_px; iy <= cy + radius_px; ++iy) {
        int idy = iy - yBase;
        int32_t yq = yQ[idy];
        uint16_t* row = fb + iy*stride;

        // Compute row span: |x| <= sqrt(1 - y^2) using LUT; index via shifts (no divide)
        // y2 is Q30; convert to Q15 then to LUT index: idx = ((y2 >> 15) >> 5)
        int32_t y2 = (int32_t)((int64_t)yq * (int64_t)yq);  // if you prefer, cast to int32: yq*yq fits in 31-bit
        int      idxY = (int)(( (y2 >> 15) ) >> 5);         // 0..1024
        if (idxY > 1024) idxY = 1024;
        int32_t xlim_q15 = SQRT_1_MINUS[idxY];
        int     xlim_px  = (int)(( (int32_t)xlim_q15 * radius_px + (Q15_ONE-1) ) / Q15_ONE);

        int xStart = cx - xlim_px;
        int xEnd   = cx + xlim_px;
        int xIdx0  = xStart - xBase;

        // Precompute longitude threshold using cos(lat) = sqrt(1 - vy^2)
        // We'll compute vy per pixel, but thr uses cos(lat) only, so we can build it after vy is known.
        for (int ix = xStart, xIdx = xIdx0; ix <= xEnd; ++ix, ++xIdx) {
            int32_t xq = xQ[xIdx];

            // r2 index for z LUT: r2_idx = (((x^2 + y^2) >> 15) >> 5);
            int32_t x2 = (int32_t)(xq * xq);
            int32_t r2q15 = (int32_t)((x2 + (int32_t)(yq * yq)) >> 15);
            int      r2_idx = (int)(r2q15 >> 5);   // 0..1024
            if (r2_idx > 1024) r2_idx = 1024;
            int32_t zq = SQRT_1_MINUS[r2_idx];

            // v_nav = R^T * v_cam
            int32_t vx = q15_mul(RT[0], xq) + q15_mul(RT[1], yq) + q15_mul(RT[2], zq);
            int32_t vy = q15_mul(RT[3], xq) + q15_mul(RT[4], yq) + q15_mul(RT[5], zq);
            int32_t vz = q15_mul(RT[6], xq) + q15_mul(RT[7], yq) + q15_mul(RT[8], zq);

            // base color from |vy| using prebuilt ramps
            uint16_t t8 = (uint16_t)(((vy<0?-vy:vy) * 255) / Q15_ONE);
            uint16_t base = (vy >= 0) ? SKY_RAMP[t8] : GND_RAMP[t8];

            if (draw_grid) {
                int on_line = 0;

                // horizon band |vy| < eps_hor
                if ((vy < 0 ? -vy : vy) < eps_hor) {
                    on_line = 1;
                } else {
                    // latitude lines |vy - sin(lat)| < eps_grid
                    for (int k=0;k<4 && !on_line;k++){
                        int32_t d = vy - lat_sin[k];
                        if (d < 0) d = -d;
                        if (d < eps_grid) on_line = 1;
                    }
                    // longitude lines around Y using XZ plane:
                    // distance ~ |vx*cos(m) - vz*sin(m)| < eps * cos(lat)
                    if (!on_line) {
                        int32_t vy2 = (int32_t)(vy * vy);
                        int idxVy = (int)(( (vy2 >> 15) ) >> 5);
                        if (idxVy > 1024) idxVy = 1024;
                        int32_t cos_lat = SQRT_1_MINUS[idxVy];  // Q15
                        int32_t thr = (int32_t)(((int32_t)eps_grid * cos_lat) >> 15);
                        for (int m=0;m<12; m++){
                            int32_t d = q15_mul(vx, lon_cos[m]) - q15_mul(vz, lon_sin[m]);
                            if (d < 0) d = -d;
                            if (d < thr) { on_line = 1; break; }
                        }
                    }
                }

                if (on_line) {
                    // brighten toward LINE_RGB (no float)
                    uint8_t r = ((base >> 11) & 0x1F) << 3;
                    uint8_t g = ((base >> 5)  & 0x3F) << 2;
                    uint8_t b = ( base        & 0x1F) << 3;
                    r = (uint8_t)((r*60 + LINE_RGB[0]*40)/100);
                    g = (uint8_t)((g*60 + LINE_RGB[1]*40)/100);
                    b = (uint8_t)((b*60 + LINE_RGB[2]*40)/100);
                    base = pack_rgb565(r,g,b);
                }
            }

            row[ix] = base;
        }
    }

    // rim (1–2 px)
    // int r_out = radius_px;
    // int r_in  = radius_px - 2;
    // white = pack_rgb565(255,255,255);
    // for (int y = cy - radius_px; y <= cy + radius_px; ++y) {
    //     uint16_t* row = fb + y*stride;
    //     for (int x = cx - radius_px; x <= cx + radius_px; ++x) {
    //         int dx = x - cx, dy = y - cy;
    //         int r2 = dx*dx + dy*dy;
    //         if (r2 <= r_out*r_out && r2 >= r_in*r_in)
    //             row[x] = white;
    //     }
    // }

    // 12 o'clock marker
    int tip_x = cx, tip_y = cy - radius_px + 6;
    int left_x = cx - 8, left_y = cy - radius_px + 14;
    int right_x= cx + 8, right_y= cy - radius_px + 14;
    fill_tri_rgb565(fb, W,H,stride, tip_x,tip_y, left_x,left_y, right_x,right_y, white);
}


// Funciton for screen
static void lcd_write_cmd(uint8_t cmd) {
    dc_command(); cs_select();
    spi_write_blocking(SPI_PORT, &cmd, 1);
    cs_deselect();
}
static void lcd_write_data(const uint8_t *data, size_t len) {
    if (!len) return;
    dc_data(); cs_select();
    spi_write_blocking(SPI_PORT, data, len);
    cs_deselect();
}

static void lcd_reset(void) {
    gpio_put(PIN_LCD_RST, 0); sleep_ms(50);
    gpio_put(PIN_LCD_RST, 1); sleep_ms(120);
}

// Parser that sends each {cmd, len, [data...] | delay}
static void lcd_run_init_sequence(const uint8_t *seq, size_t len) {
    size_t i = 0;
    while (i < len) {
        uint8_t cmd = seq[i++];

        if (i >= len) break;
        uint8_t n = seq[i++];

        if (n & DELAY_FLAG) {
            // delay-only entry
            uint8_t ms = seq[i++];
            lcd_write_cmd(cmd);
            sleep_ms(ms);
        } else {
            // n data bytes follow
            lcd_write_cmd(cmd);
            if (n) lcd_write_data(&seq[i], n);
            i += n;
        }
    }
}

static void lcd_init(void) {
    // Basic bring-up for GC9A01A (minimal but works for RGB565 full-screen)
    lcd_reset();

    lcd_run_init_sequence(gc9a01_init, sizeof(gc9a01_init));
}

static void lcd_set_window_full(void) {
    // For round 240x240, set full square window: X:0..239, Y:0..239
    uint8_t col_addr[] = {0x00, 0x00, 0x00, (uint8_t)(LCD_WIDTH - 1)};
    uint8_t row_addr[] = {0x00, 0x00, 0x00, (uint8_t)(LCD_HEIGHT - 1)};
    lcd_write_cmd(0x2A); lcd_write_data(col_addr, 4);  // CASET
    lcd_write_cmd(0x2B); lcd_write_data(row_addr, 4);  // RASET
    lcd_write_cmd(0x2C);                                // RAMWR (next write = pixel data)
}

static inline uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b) {
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

void __isr dma_irq0_handler(void) {
    // Which channels triggered?
    uint32_t ints = dma_hw->ints0;

    if (ints & (1u << lcd_dma_chan)) {
        // Clear the interrupt for our channel
        dma_hw->ints0 = 1u << lcd_dma_chan;

        // DMA is done feeding the SPI FIFO. Make sure SPI shifted everything out.
        while (spi_is_busy(SPI_PORT)) {
            tight_loop_contents();
        }

        // Deassert CS and restore SPI format if needed
        cs_deselect();
        spi_set_format(SPI_PORT, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);

        // Signal completion to main code
        lcd_frame_done = true;
    }
}

void lcd_dma_init(void) {
    lcd_dma_chan = dma_claim_unused_channel(true);

    dma_channel_config c = dma_channel_get_default_config(lcd_dma_chan);
    channel_config_set_transfer_data_size(&c, DMA_SIZE_16);                  // 16-bit per transfer
    channel_config_set_read_increment(&c, true);                             // increment framebuffer pointer
    channel_config_set_write_increment(&c, false);                           // always write to same SPI DR
    channel_config_set_dreq(&c, spi_get_dreq(SPI_PORT, true));               // pace by SPI TX DREQ

    dma_channel_configure(
        lcd_dma_chan,
        &c,
        &spi_get_hw(SPI_PORT)->dr,  // write address (SPI data register)
        NULL,                       // read address set later
        0,                          // transfer count set later
        false                       // don't start yet
    );

    // Attach our handler to DMA_IRQ_0
    irq_set_exclusive_handler(DMA_IRQ_0, dma_irq0_handler);
    irq_set_enabled(DMA_IRQ_0, true);

    // Enable IRQ for this channel on IRQ0
    dma_channel_set_irq0_enabled(lcd_dma_chan, true);
}

static void lcd_push_framebuffer_dma(void) {
    // lcd_set_window_full();

    // dc_data();
    // cs_select();

    // // Switch SPI to 16-bit transfers so each pixel can go as one word
    // // (MSB-first, so the panel still sees hi-byte then lo-byte)
    // spi_set_format(SPI_PORT, 16, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);

    // uint32_t pixel_count = (uint32_t)LCD_WIDTH * (uint32_t)LCD_HEIGHT;

    // // Configure DMA for this frame
    // dma_channel_set_read_addr(lcd_dma_chan, fb_to_show, false);
    // dma_channel_set_trans_count(lcd_dma_chan, pixel_count, true); // this also starts the transfer

    // // Wait for DMA to finish moving all words into the SPI TX FIFO
    // dma_channel_wait_for_finish_blocking(lcd_dma_chan);

    // // Make sure SPI has actually shifted out everything
    // while (spi_is_busy(SPI_PORT)) {
    //     tight_loop_contents();
    // }

    // cs_deselect();

    // // Restore SPI to 8-bit mode for commands / other traffic, if needed
    // spi_set_format(SPI_PORT, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);

    lcd_set_window_full();

    dc_data();
    cs_select();

    // 16-bit SPI for RGB565 pixels
    spi_set_format(SPI_PORT, 16, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);

    uint32_t pixel_count = (uint32_t)LCD_WIDTH * (uint32_t)LCD_HEIGHT;

    lcd_frame_done = false;

    dma_channel_set_read_addr(lcd_dma_chan, fb_to_show, false);
    dma_channel_set_trans_count(lcd_dma_chan, pixel_count, true);  // starts DMA
    // DMA IRQ will signal when done
}

/**********************************************************************/
/*-------------------------- PROTOTYPES ------------------------------*/
/**********************************************************************/
void core_task1(void);

/**********************************************************************/
/*--------------------------- MAIN FUNCTION -------------------------*/
/**********************************************************************/

int main() {
    stdio_init_all();

    uint16_t yaw = 0, pitch = 0, roll = 0;
    init_luts(); // Called once at startup
    build_color_ramps(); // Prebuild color ramps

    spi_init(SPI_PORT, 40500000); // 40.5 MHz (tune down if unstable)
    gpio_set_function(PIN_LCD_SCK,  GPIO_FUNC_SPI);
    gpio_set_function(PIN_LCD_MOSI, GPIO_FUNC_SPI);
    // CS / DC / RST / BL as GPIO
    gpio_init(PIN_LCD_CS);  gpio_set_dir(PIN_LCD_CS, GPIO_OUT);  gpio_put(PIN_LCD_CS, 1);
    gpio_init(PIN_LCD_DC);  gpio_set_dir(PIN_LCD_DC, GPIO_OUT);  gpio_put(PIN_LCD_DC, 1);
    gpio_init(PIN_LCD_RST); gpio_set_dir(PIN_LCD_RST, GPIO_OUT); gpio_put(PIN_LCD_RST, 1);
    gpio_init(PIN_LCD_BL);  gpio_set_dir(PIN_LCD_BL, GPIO_OUT);  gpio_put(PIN_LCD_BL, 1); // backlight on
    gpio_init(PIN_LCD_MISO); gpio_set_dir(PIN_LCD_MISO, GPIO_IN);

    lcd_init();
    lcd_dma_init();

    // multicore_launch_core1(core_task1);

    lcd_frame_done = true;

    while (1) {

        navball_render_rgb565(fb_to_draw, WIDTH, HEIGHT, STRIDE, yaw, pitch, roll, ENABLE_GRID);

        // Swap framebuffers, so dma sends the one we just drew, while we draw to the other
        uint16_t* temp = fb_to_show;
        fb_to_show = fb_to_draw;
        fb_to_draw = temp;

        // Simple animation
        yaw   = (yaw   + 5) % 360;
        pitch = (pitch + 3) % 360;
        roll  = (roll  + 7) % 360;

        while (!lcd_frame_done) {
            tight_loop_contents();
        }
        
        lcd_frame_done = false;
        lcd_push_framebuffer_dma();
    }

    return 0;
}



/**********************************************************************/
/*------------------------ CORE 1 FUNCTION --------------------------*/
/**********************************************************************/
void core_task1(void)
{
    while (true) {

    }
}