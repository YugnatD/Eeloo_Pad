/* Navball renderer, fixed-point Q15.
 * Same algorithm as navball/rp2040/main.c (sphere shading, horizon,
 * latitude/longitude grid), generalised to large round panels:
 *  - the sqrt(1 - t) table has 4097 entries instead of 1025 (no banding
 *    near the rim at this radius),
 *  - inner loop optimised for the ESP32-P4 (meridians folded by symmetry, no per-pixel
 *    bounds checks, table lookups instead of branches),
 *  - line thickness is given in pixels and scaled with the radius. */
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "navball.h"

#define Q15_ONE 32767
#define CLAMP(v, a, b) ((v) < (a) ? (a) : ((v) > (b) ? (b) : (v)))

#define SQRT_BITS 12                    /* table index = (r^2 in Q15) >> (15-SQRT_BITS) */
#define SQRT_N    (1 << SQRT_BITS)

/* colours (8-bit channels), identical to the RP2040 version */
static const uint8_t SKY_ZENITH[3]  = { 83, 155, 245};
static const uint8_t SKY_HORIZON[3] = { 33, 104, 214};
static const uint8_t GND_HORIZON[3] = {172, 108,  58};
static const uint8_t GND_NADIR[3]   = {112,  70,  35};
static const uint8_t LINE_RGB[3]    = {245, 245, 245};

/* [0..255] = shaded colour, [256..511] = same colour blended with the grid line colour */
static uint16_t COLOR_TAB[1024];   /* [0] sky, [256] sky + line, [512] ground, [768] ground + line */
#define SKY_RAMP COLOR_TAB
#define SKY_LINE (COLOR_TAB + 256)
#define GND_RAMP (COLOR_TAB + 512)
#define GND_LINE (COLOR_TAB + 768)
static int16_t SIN_DEG[360], COS_DEG[360];
static int16_t SQRT_1_MINUS[SQRT_N + 256];   /* + guard entries (= 0) for rounding above 1.0 */
static int32_t xQ[NAVBALL_MAX_SIZE + 1], yQ[NAVBALL_MAX_SIZE + 1];
#ifndef NAVBALL_MERIDIAN_HALF_RATE
#define NAVBALL_MERIDIAN_HALF_RATE 1   /* meridian test every other pixel, interpolated */
#endif
#define ZSH 5                          /* sqrt table index = (X^2 + Y^2) >> ZSH */
#define ZTAB_N ((NAVBALL_MAX_SIZE / 2) * (NAVBALL_MAX_SIZE / 2) >> ZSH)
static uint16_t ZTAB[ZTAB_N + 256];    /* sqrt(1 - S / r^2) in Q15, S = X^2 + Y^2 in pixels */
/* shade table, index = (|vy| >> 5) | meridian << 11 | ground << 12:
 * shaded colour, or line colour on the horizon / +-30 / +-60 parallels / meridians */
static uint16_t SHADE_TAB[8192];
static int32_t shade_eps_grid = -1, shade_eps_hor = -1;
static int cache_radius = -1, cache_cx, cache_cy;

/* ---------- SIMD path (ESP32-P4 PIE, 8 x int16 lanes), see navball_simd.S ---------- */
#if defined(ESP_PLATFORM) && defined(CONFIG_IDF_TARGET_ESP32P4)
#include "esp_cpu.h"
#define CORE_ID() esp_cpu_get_core_id()
void navball_simd_idx(const int16_t *xq, const int16_t *zq, int16_t *idx, int n8, const int16_t *k);
#define SIMD_KERNEL navball_simd_idx
#else
#define CORE_ID() 0
#define SIMD_KERNEL navball_simd_idx_ref
#endif
#ifndef NAVBALL_SIMD
#define NAVBALL_SIMD 1
#endif
#define SCRATCH_N (NAVBALL_MAX_SIZE + 32)
static int16_t XQ16[SCRATCH_N] __attribute__((aligned(16)));         /* (col - cx) / r, Q15 */
static int16_t ZROW[2][SCRATCH_N] __attribute__((aligned(16)));      /* per core */
static int16_t IROW[2][SCRATCH_N] __attribute__((aligned(16)));

static inline int16_t sat16(int32_t v) { return v > 32767 ? 32767 : v < -32768 ? -32768 : (int16_t)v; }
static inline int16_t vmul(int16_t a, int16_t b) { return sat16(((int32_t)a * b) >> 15); }
static inline int16_t vadd(int16_t a, int16_t b) { return sat16((int32_t)a + b); }
static inline int16_t vabs(int16_t a) { return sat16(a < 0 ? -(int32_t)a : a); }

/* C reference of navball_simd.S, lane by lane, same saturating int16 arithmetic */
void navball_simd_idx_ref(const int16_t *xq, const int16_t *zq, int16_t *idx, int n8, const int16_t *k)
{
    const int16_t R3 = k[0], R5 = k[1], BY = k[2], K1024 = k[4], M4096 = k[5], R0 = k[6], R2 = k[7], AY = k[8];
    const int16_t R6 = k[9], R8 = k[10], CYY = k[11], C30 = k[12], HALF = k[13], EPS = k[14], M2048 = k[15];
    for (int i = 0; i < n8 * 8; i++) {
        int16_t vy = vadd(vadd(vmul(xq[i], R3), vmul(zq[i], R5)), BY);
        int16_t neg = vy < 0 ? -1 : 0;
        int16_t id = vmul(vabs(vy), K1024) | (neg & M4096);
        int16_t a = vabs(vadd(vadd(vmul(xq[i], R0), vmul(zq[i], R2)), AY));
        int16_t b = vabs(vadd(vadd(vmul(xq[i], R6), vmul(zq[i], R8)), CYY));
        int16_t hi = a > b ? a : b, lo = a < b ? a : b;
        int16_t d = vabs(sat16((int32_t)vmul(lo, C30) - vmul(hi, HALF)));
        int16_t on = (lo < EPS ? -1 : 0) | (d < EPS ? -1 : 0);
        idx[i] = id | (on & M2048);
    }
}


static inline int32_t q15_mul(int32_t a, int32_t b) { return (a * b + (1 << 14)) >> 15; }

static inline uint16_t pack_rgb565(int r, int g, int b)
{
    return (uint16_t)(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
}

static inline int sqrt_idx(int32_t q15)        /* q15 in [0, 1] -> table index */
{
    int i = q15 >> (15 - SQRT_BITS);
    return i > SQRT_N ? SQRT_N : i;
}

void navball_init(void)
{
    for (int d = 0; d < 360; ++d) {
        double rad = d * (M_PI / 180.0);
        SIN_DEG[d] = (int16_t)CLAMP(lround(sin(rad) * Q15_ONE), -Q15_ONE, Q15_ONE);
        COS_DEG[d] = (int16_t)CLAMP(lround(cos(rad) * Q15_ONE), -Q15_ONE, Q15_ONE);
    }
    for (int i = 0; i <= SQRT_N; ++i) {
        double t = (double)i / SQRT_N;
        SQRT_1_MINUS[i] = (int16_t)CLAMP(lround(sqrt(fmax(0.0, 1.0 - t)) * Q15_ONE), 0, Q15_ONE);
    }
    for (int t = 0; t < 256; ++t) {
        int c[2][3];
        for (int k = 0; k < 3; ++k) {
            c[0][k] = (SKY_HORIZON[k] * (255 - t) + SKY_ZENITH[k] * t) / 255;
            c[1][k] = (GND_HORIZON[k] * (255 - t) + GND_NADIR[k] * t) / 255;
        }
        SKY_RAMP[t] = pack_rgb565(c[0][0], c[0][1], c[0][2]);
        GND_RAMP[t] = pack_rgb565(c[1][0], c[1][1], c[1][2]);
        /* line = 60 % base + 40 % white, precomputed instead of per pixel */
        SKY_LINE[t] = pack_rgb565((c[0][0] * 60 + LINE_RGB[0] * 40) / 100,
                                  (c[0][1] * 60 + LINE_RGB[1] * 40) / 100,
                                  (c[0][2] * 60 + LINE_RGB[2] * 40) / 100);
        GND_LINE[t] = pack_rgb565((c[1][0] * 60 + LINE_RGB[0] * 40) / 100,
                                  (c[1][1] * 60 + LINE_RGB[1] * 40) / 100,
                                  (c[1][2] * 60 + LINE_RGB[2] * 40) / 100);
    }
}

/* R = Rz(yaw) * Ry(pitch) * Rx(roll), Q15 — same convention as the RP2040 code */
/* Navball frame: X = east, Y = up, Z = north. Camera frame: x = vessel right, y = vessel up (top
 * of the screen), z = vessel nose (the centre of the ball). M is camera -> navball: its columns are
 * the vessel axes (right, up, nose) expressed in the navball frame. KSP conventions: heading from
 * north towards east, pitch above the horizon, roll positive = right wing down. */
void navball_attitude_matrix(float heading, float pitch, float roll, float M[9])
{
    const float k = (float)M_PI / 180.0f;
    const float sh = sinf(heading * k), ch = cosf(heading * k);
    const float sp = sinf(pitch * k), cp = cosf(pitch * k);
    const float sr = sinf(roll * k), cr = cosf(roll * k);
    const float f[3] = {sh * cp, sp, ch * cp};            /* nose */
    const float r0[3] = {ch, 0, -sh};                     /* right, wings level */
    const float u0[3] = {f[1] * r0[2] - f[2] * r0[1],     /* up = nose x right */
                         f[2] * r0[0] - f[0] * r0[2],
                         f[0] * r0[1] - f[1] * r0[0]};
    for (int i = 0; i < 3; i++) {
        M[3 * i + 0] = cr * r0[i] - sr * u0[i];          /* right */
        M[3 * i + 1] = sr * r0[i] + cr * u0[i];          /* up */
        M[3 * i + 2] = f[i];                              /* nose */
    }
}

/* unit vector of a direction given as heading / pitch, in the navball frame */
void navball_direction(float heading, float pitch, float d[3])
{
    const float k = (float)M_PI / 180.0f;
    d[0] = sinf(heading * k) * cosf(pitch * k);
    d[1] = sinf(pitch * k);
    d[2] = cosf(heading * k) * cosf(pitch * k);
}

void navball_prepare(navball_ctx_t *c, uint16_t *fb, int W, int H, int stride,
                     const float M[9], int draw_grid)
{
    int D = W < H ? W : H;
    if (D > NAVBALL_MAX_SIZE) D = NAVBALL_MAX_SIZE;
    c->fb = fb; c->W = W; c->H = H; c->stride = stride;
    c->cx = W / 2; c->cy = H / 2;
    c->radius_px = (D - 2) / 2;
    c->xBase = c->cx - c->radius_px;
    c->yBase = c->cy - c->radius_px;
    c->draw_grid = draw_grid;

    /* line half-thickness in Q15 ~ px / radius; 1 px grid / 2 px horizon at
     * 240 px (RP2040), scaled up so lines keep the same look at 800 px */
    float scale = c->radius_px / 119.0f;
    float grid_px = 1.0f * scale * 0.6f, hor_px = 2.0f * scale * 0.6f;
    c->eps_grid = (int32_t)(grid_px * Q15_ONE / c->radius_px);
    c->eps_hor  = (int32_t)(hor_px  * Q15_ONE / c->radius_px);

    /* the renderer computes v_nav = RT * v_cam with RT in Q15 */
    for (int i = 0; i < 9; i++) c->RT[i] = (int32_t)CLAMP(lroundf(M[i] * Q15_ONE), -Q15_ONE, Q15_ONE);

    if (cache_radius != c->radius_px || cache_cx != c->cx || cache_cy != c->cy) {
        for (int i = 0; i <= 2 * c->radius_px; ++i) {
            xQ[i] = ((i - c->radius_px) * Q15_ONE) / c->radius_px;   /* (x - cx) / r */
            yQ[i] = ((c->radius_px - i) * Q15_ONE) / c->radius_px;   /* +Y up */
        }
        for (int col = 0; col < SCRATCH_N; ++col)
            XQ16[col] = (int16_t)CLAMP(((col - c->cx) * Q15_ONE) / c->radius_px, -32768, 32767);
        int r2 = c->radius_px * c->radius_px;
        for (int i = 0; i < ZTAB_N + 256; ++i) {
            double t = (((double)i + 0.5) * (1 << ZSH)) / r2;          /* bucket centre */
            ZTAB[i] = t >= 1.0 ? 0 : (uint16_t)CLAMP(lround(sqrt(1.0 - t) * Q15_ONE), 0, Q15_ONE);
        }
        cache_radius = c->radius_px; cache_cx = c->cx; cache_cy = c->cy;
    }
    int32_t eg = c->draw_grid ? c->eps_grid : 0, eh = c->draw_grid ? c->eps_hor : 0;
    if (eg != shade_eps_grid || eh != shade_eps_hor) {
        for (int i = 0; i < 2048; i++) {
            int32_t avy = i * 32 + 16;
            if (avy > Q15_ONE) avy = Q15_ONE;
            int t8 = avy >> 7;
            int lat = avy < eh || abs(avy - SIN_DEG[30]) < eg || abs(avy - SIN_DEG[60]) < eg;
            SHADE_TAB[i]        = lat ? SKY_LINE[t8] : SKY_RAMP[t8];
            SHADE_TAB[i | 2048] = SKY_LINE[t8];
            SHADE_TAB[i | 4096] = lat ? GND_LINE[t8] : GND_RAMP[t8];
            SHADE_TAB[i | 6144] = GND_LINE[t8];
        }
        shade_eps_grid = eg; shade_eps_hor = eh;
    }
    static const int lat_deg[4] = {-60, -30, 30, 60};
    for (int i = 0; i < 4; i++) c->lat_sin[i] = SIN_DEG[(lat_deg[i] + 360) % 360];
    for (int i = 0; i < 12; i++) { c->lon_sin[i] = SIN_DEG[i * 30]; c->lon_cos[i] = COS_DEG[i * 30]; }
}

/* meridians every 30 deg: fold (vx, vz) into [0, 45] deg by symmetry (|.|, then sort), only the
 * 0 deg and 30 deg meridians remain (same lines as testing all six). Returns 1 on a line. */
static inline uint32_t meridian_on(int32_t a, int32_t b, int32_t C30, int32_t eps, uint32_t two_eps)
{
    int32_t m = a >> 31; a = (a ^ m) - m;
    m = b >> 31;         b = (b ^ m) - m;
    const int32_t diff = a - b, dm = diff >> 31;
    const int32_t hi = a - (diff & dm), lo = b + (diff & dm);
    const int32_t d = ((lo * C30) >> 15) - (hi >> 1);              /* sin 30 = 1/2 */
    return (lo < eps) | ((uint32_t)(d + eps) < two_eps);
}

static inline int isqrt32(int32_t v)       /* floor(sqrt(v)), v >= 0 */
{
    int r = (int)sqrtf((float)v);
    while (r * r > v) r--;
    while ((r + 1) * (r + 1) <= v) r++;
    return r;
}

void navball_render_rows(const navball_ctx_t *c, int y_start, int y_end)
{
    if (y_start < c->cy - c->radius_px) y_start = c->cy - c->radius_px;
    if (y_end > c->cy + c->radius_px) y_end = c->cy + c->radius_px;
    if (y_end < y_start) return;
    navball_render_rows_to(c, y_start, y_end, c->fb + y_start * c->stride, c->stride, 0);
}

/* Render rows [y_start, y_end] into dst (row y_start at dst, dst_stride pixels per row).
 * clear != 0: full rows are written (black outside the ball), for strip buffers. */
void navball_render_rows_to(const navball_ctx_t *c, int y_start, int y_end, uint16_t *dst, int dst_stride, int clear)
{
    const int cx = c->cx, cy = c->cy, r = c->radius_px, rr = r * r;
    const int32_t R2 = c->RT[2], R5 = c->RT[5], R8 = c->RT[8];
    /* per-pixel steps of RT[k] * x / r in Q29 (Q15 << 14): the x terms advance by additions */
    const int32_t s0 = (c->RT[0] << 14) / r, s3 = (c->RT[3] << 14) / r, s6 = (c->RT[6] << 14) / r;
    const int32_t eps = c->draw_grid ? c->eps_grid : 0;
    const uint32_t two_eps = 2u * (uint32_t)eps;
    const int32_t C30 = COS_DEG[30];
    const uint16_t *ztab = ZTAB, *tab = SHADE_TAB;

    for (int iy = y_start; iy <= y_end; ++iy) {
        uint16_t *rowp = dst + (iy - y_start) * dst_stride;
        const int Y = cy - iy;                       /* +Y up, pixels */
        if (Y > r || Y < -r) {                       /* row outside the ball */
            if (clear) memset(rowp, 0, c->W * 2);
            continue;
        }
        const int32_t yq = yQ[iy - c->yBase];
        const int xlim = isqrt32(rr - Y * Y);
        if (clear) {                                 /* black margins left and right of the span */
            memset(rowp, 0, (cx - xlim) * 2);
            memset(rowp + cx + xlim + 1, 0, (c->W - (cx + xlim + 1)) * 2);
        }
#if NAVBALL_SIMD
        {
            const int core = CORE_ID();
            int16_t *zr = ZROW[core], *ir = IROW[core];
            const int xs = cx - xlim, xa = xs & ~7;          /* span start, aligned down to 8 pixels */
            const int n8 = ((cx + xlim) - xa) / 8 + 1;
            /* 1. z for every pixel of the aligned span (scalar table lookups) */
            int Xa = xa - cx;
            uint32_t Sa = (uint32_t)(Xa * Xa + Y * Y);
            int32_t ta = 2 * Xa + 1;
            for (int j = 0; j < n8 * 8; j++) { zr[j] = (int16_t)ZTAB[Sa >> ZSH]; Sa += ta; ta += 2; }
            /* 2. shade index for 8 pixels at a time (SIMD) */
            const int16_t kk[16] = {
                (int16_t)c->RT[3], (int16_t)c->RT[5], (int16_t)q15_mul(c->RT[4], yq), 0, 1024, 4096,
                (int16_t)c->RT[0], (int16_t)c->RT[2], (int16_t)q15_mul(c->RT[1], yq),
                (int16_t)c->RT[6], (int16_t)c->RT[8], (int16_t)q15_mul(c->RT[7], yq),
                (int16_t)C30, 16384, (int16_t)eps, 2048 };
            SIMD_KERNEL(&XQ16[xa], zr, ir, n8, kk);
            /* 3. colours of the exact span */
            const int16_t *ip = ir + (xs - xa);
            uint16_t *px = rowp + xs;
            for (int n = 2 * xlim + 1; n > 0; --n) *px++ = tab[*ip++];
            continue;
        }
#endif
        const int X0 = -xlim;
        uint32_t S = (uint32_t)(X0 * X0 + Y * Y);    /* X^2 + Y^2, advanced by t = 2X + 1 */
        int32_t t = 2 * X0 + 1;

        /* v = RT * (x, y, z): the y terms are folded into the start values of the x accumulators */
        int32_t p0 = X0 * s0 + (q15_mul(c->RT[1], yq) << 14);
        int32_t p3 = X0 * s3 + (q15_mul(c->RT[4], yq) << 14);
        int32_t p6 = X0 * s6 + (q15_mul(c->RT[7], yq) << 14);
        uint16_t *px = rowp + cx + X0;

#if NAVBALL_MERIDIAN_HALF_RATE
        /* Meridian components (a, b) are computed every other pixel; the pixel in between uses
         * their average (they vary smoothly). Shading, horizon and parallels stay per pixel. */
        /* first pixel (an "even" one), then pairs: odd pixel (interpolated), even pixel (computed) */
        int32_t zq = (int16_t)ztab[S >> ZSH];
        S += t; t += 2;
        int32_t vy = (p3 >> 14) + ((R5 * zq) >> 15), sy = vy >> 31;
        int32_t a_prev = (p0 >> 14) + ((R2 * zq) >> 15), b_prev = (p6 >> 14) + ((R8 * zq) >> 15);
        *px++ = tab[(((vy ^ sy) - sy) >> 5) | (sy & 4096) | (meridian_on(a_prev, b_prev, C30, eps, two_eps) << 11)];
        p0 += s0; p3 += s3; p6 += s6;
        for (int k = xlim; k > 0; --k) {
            /* odd pixel: shading now, meridian test once the next even pixel is known */
            zq = (int16_t)ztab[S >> ZSH];
            S += t; t += 2;
            vy = (p3 >> 14) + ((R5 * zq) >> 15); sy = vy >> 31;
            const int32_t idx_odd = (((vy ^ sy) - sy) >> 5) | (sy & 4096);
            p0 += s0; p3 += s3; p6 += s6;
            /* even pixel: full computation */
            zq = (int16_t)ztab[S >> ZSH];
            S += t; t += 2;
            vy = (p3 >> 14) + ((R5 * zq) >> 15); sy = vy >> 31;
            const int32_t a = (p0 >> 14) + ((R2 * zq) >> 15), b = (p6 >> 14) + ((R8 * zq) >> 15);
            px[0] = tab[idx_odd | (meridian_on((a + a_prev) >> 1, (b + b_prev) >> 1, C30, eps, two_eps) << 11)];
            px[1] = tab[(((vy ^ sy) - sy) >> 5) | (sy & 4096) | (meridian_on(a, b, C30, eps, two_eps) << 11)];
            px += 2;
            a_prev = a; b_prev = b;
            p0 += s0; p3 += s3; p6 += s6;
        }
#else
        uint16_t *const end = px + 2 * xlim + 1;
        while (px < end) {
            /* branch-free, register-only inner loop (~45 instructions per pixel) */
            const int32_t zq = (int16_t)ztab[S >> ZSH];  /* sqrt(1 - (X^2+Y^2)/r^2), Q15 */
            S += t; t += 2;

            const int32_t vy = (p3 >> 14) + ((R5 * zq) >> 15);
            const int32_t sy = vy >> 31;                 /* 0 = sky, -1 = ground */
            const int32_t avy = (vy ^ sy) - sy;

            /* meridians every 30 deg: fold (vx, vz) into [0, 45] deg by symmetry (|.|, then sort),
             * only the 0 deg and 30 deg meridians remain (same lines as testing all six) */
            int32_t a = (p0 >> 14) + ((R2 * zq) >> 15);
            int32_t b = (p6 >> 14) + ((R8 * zq) >> 15);
            int32_t m = a >> 31; a = (a ^ m) - m;
            m = b >> 31;         b = (b ^ m) - m;
            const int32_t diff = a - b, dm = diff >> 31;
            const int32_t hi = a - (diff & dm), lo = b + (diff & dm);
            const int32_t d = ((lo * C30) >> 15) - (hi >> 1);          /* sin 30 = 1/2 */
            const uint32_t on = (lo < eps) | ((uint32_t)(d + eps) < two_eps);

            p0 += s0; p3 += s3; p6 += s6;
            /* shade table: |vy| (horizon and parallels already drawn in it), meridian bit, sky/ground */
            *px++ = tab[(avy >> 5) | (on << 11) | (sy & 4096)];
        }
#endif
    }
}

/* Compare the SIMD kernel with its C reference on a few rows; returns the number of differing
 * pixels (0 expected). Run once at start-up on the target. */
int navball_simd_selftest(const navball_ctx_t *c)
{
    int bad = 0;
    static int16_t ref[SCRATCH_N] __attribute__((aligned(16)));
    const int r = c->radius_px;
    for (int Y = -r + 3; Y < r; Y += r / 7) {
        const int xlim = isqrt32(r * r - Y * Y), xa = (c->cx - xlim) & ~7, n8 = ((c->cx + xlim) - xa) / 8 + 1;
        int16_t *zr = ZROW[0], *ir = IROW[0];
        int Xa = xa - c->cx; uint32_t Sa = (uint32_t)(Xa * Xa + Y * Y); int32_t ta = 2 * Xa + 1;
        for (int j = 0; j < n8 * 8; j++) { zr[j] = (int16_t)ZTAB[Sa >> ZSH]; Sa += ta; ta += 2; }
        const int32_t yq = (Y * Q15_ONE) / r;
        const int16_t kk[16] = {
            (int16_t)c->RT[3], (int16_t)c->RT[5], (int16_t)q15_mul(c->RT[4], yq), 0, 1024, 4096,
            (int16_t)c->RT[0], (int16_t)c->RT[2], (int16_t)q15_mul(c->RT[1], yq),
            (int16_t)c->RT[6], (int16_t)c->RT[8], (int16_t)q15_mul(c->RT[7], yq),
            (int16_t)COS_DEG[30], 16384, (int16_t)c->eps_grid, 2048 };
        SIMD_KERNEL(&XQ16[xa], zr, ir, n8, kk);
        navball_simd_idx_ref(&XQ16[xa], zr, ref, n8, kk);
        for (int j = 0; j < n8 * 8; j++) bad += ir[j] != ref[j];
    }
    return bad;
}
