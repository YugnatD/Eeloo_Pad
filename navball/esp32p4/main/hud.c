/* Navball markers: procedural anti-aliased sprites (colour + alpha), built once at start-up,
 * then blended over the ball row by row inside the render strips (internal SRAM). */
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "hud.h"
#include "navball_font.h"

#define MS 64                       /* marker sprite size */
#define LW 112                      /* level symbol sprite */
#define LH 40

typedef struct { int w, h; uint16_t *col; uint8_t *alpha; } sprite_t;
static sprite_t spr[MK_COUNT];

/* ---------- tiny 2D distance-field painter ---------- */
typedef struct { float *cov; float *dark; int w, h; } canvas_t;

static float seg_dist(float px, float py, float ax, float ay, float bx, float by)
{
    float vx = bx - ax, vy = by - ay, wx = px - ax, wy = py - ay;
    float t = (wx * vx + wy * vy) / (vx * vx + vy * vy);
    t = t < 0 ? 0 : t > 1 ? 1 : t;
    float dx = wx - t * vx, dy = wy - t * vy;
    return sqrtf(dx * dx + dy * dy);
}
static inline float coverage(float d, float half_w) { float c = half_w + 0.5f - d; return c < 0 ? 0 : c > 1 ? 1 : c; }
static inline void put(canvas_t *cv, int i, float d, float half_w)
{
    float c = coverage(d, half_w), o = coverage(d, half_w + 1.6f);   /* dark outline for contrast */
    if (c > cv->cov[i]) cv->cov[i] = c;
    if (o > cv->dark[i]) cv->dark[i] = o;
}
static void line(canvas_t *cv, float ax, float ay, float bx, float by, float w)
{
    for (int y = 0; y < cv->h; y++)
        for (int x = 0; x < cv->w; x++)
            put(cv, y * cv->w + x, seg_dist(x + 0.5f, y + 0.5f, ax, ay, bx, by), w / 2);
}
static void ring(canvas_t *cv, float cx, float cy, float r, float w)
{
    for (int y = 0; y < cv->h; y++)
        for (int x = 0; x < cv->w; x++) {
            float dx = x + 0.5f - cx, dy = y + 0.5f - cy;
            put(cv, y * cv->w + x, fabsf(sqrtf(dx * dx + dy * dy) - r), w / 2);
        }
}
static void dot(canvas_t *cv, float cx, float cy, float r)
{
    for (int y = 0; y < cv->h; y++)
        for (int x = 0; x < cv->w; x++) {
            float dx = x + 0.5f - cx, dy = y + 0.5f - cy;
            float d = sqrtf(dx * dx + dy * dy) - r;
            put(cv, y * cv->w + x, d < 0 ? 0 : d, 0.0f);
        }
}
/* tick from radius r0 to r1 at angle a (degrees, 0 = up, clockwise) */
static void tick(canvas_t *cv, float cx, float cy, float a, float r0, float r1, float w)
{
    float s = sinf(a * (float)M_PI / 180), c = -cosf(a * (float)M_PI / 180);
    line(cv, cx + s * r0, cy + c * r0, cx + s * r1, cy + c * r1, w);
}

static inline uint16_t rgb565(int r, int g, int b) { return (uint16_t)(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)); }

static void finish(sprite_t *sp, canvas_t *cv, int r, int g, int b)
{
    sp->w = cv->w; sp->h = cv->h;
    sp->col = malloc(cv->w * cv->h * 2);
    sp->alpha = malloc(cv->w * cv->h);
    for (int i = 0; i < cv->w * cv->h; i++) {
        float c = cv->cov[i], o = cv->dark[i];
        float a = c + (1 - c) * o * 0.7f;                     /* colour over a dark halo */
        float k = a > 0 ? c / a : 0;                          /* share of the colour in the pixel */
        sp->col[i] = rgb565((int)(r * k + 15 * (1 - k)), (int)(g * k + 15 * (1 - k)), (int)(b * k + 20 * (1 - k)));
        sp->alpha[i] = (uint8_t)(a * 255 + 0.5f);
    }
}

static void build(marker_type_t t)
{
    int w = t == MK_LEVEL ? LW : MS, h = t == MK_LEVEL ? LH : MS;
    canvas_t cv = {calloc(w * h, sizeof(float)), calloc(w * h, sizeof(float)), w, h};
    const float c = MS / 2.0f, R = 15, W = 4;
    switch (t) {
    case MK_PROGRADE:        /* KSP: circle, centre dot, three legs (up, left, right) */
        ring(&cv, c, c, R, W); dot(&cv, c, c, 3.5f);
        tick(&cv, c, c, 0, R, R + 11, W); tick(&cv, c, c, 270, R, R + 11, W); tick(&cv, c, c, 90, R, R + 11, W);
        finish(&spr[t], &cv, 225, 225, 30); break;
    case MK_RETROGRADE:      /* circle with a cross, three legs */
        ring(&cv, c, c, R, W);
        tick(&cv, c, c, 45, 0, R * 0.72f, W); tick(&cv, c, c, 225, 0, R * 0.72f, W);
        tick(&cv, c, c, 135, 0, R * 0.72f, W); tick(&cv, c, c, 315, 0, R * 0.72f, W);
        tick(&cv, c, c, 180, R, R + 11, W); tick(&cv, c, c, 300, R, R + 11, W); tick(&cv, c, c, 60, R, R + 11, W);
        finish(&spr[t], &cv, 225, 225, 30); break;
    case MK_NORMAL:          /* triangle with a centre dot */
    case MK_ANTINORMAL: {    /* triangle with three spokes to the centre */
        float tr = 19, ax[3], ay[3];
        for (int i = 0; i < 3; i++) { ax[i] = c + tr * sinf(i * 2.0944f); ay[i] = c + 3 - tr * cosf(i * 2.0944f); }
        for (int i = 0; i < 3; i++) line(&cv, ax[i], ay[i], ax[(i + 1) % 3], ay[(i + 1) % 3], W);
        if (t == MK_NORMAL) dot(&cv, c, c + 3, 3.5f);
        else for (int i = 0; i < 3; i++) line(&cv, c, c + 3, (ax[i] * 2 + c) / 3, (ay[i] * 2 + c + 3) / 3, W);
        finish(&spr[t], &cv, 200, 80, 255); break;
    }
    case MK_RADIAL_OUT:      /* circle, dot, four legs outward at 45 deg */
        ring(&cv, c, c, R, W); dot(&cv, c, c, 3.5f);
        for (int i = 0; i < 4; i++) tick(&cv, c, c, 45 + 90 * i, R, R + 10, W);
        finish(&spr[t], &cv, 60, 220, 255); break;
    case MK_RADIAL_IN:       /* circle, four legs inward at 45 deg */
        ring(&cv, c, c, R, W);
        for (int i = 0; i < 4; i++) tick(&cv, c, c, 45 + 90 * i, R * 0.25f, R, W);
        finish(&spr[t], &cv, 60, 220, 255); break;
    case MK_TARGET:          /* circle, dot, four legs outward (axes) */
        ring(&cv, c, c, R, W); dot(&cv, c, c, 3.5f);
        for (int i = 0; i < 4; i++) tick(&cv, c, c, 90 * i, R, R + 10, W);
        finish(&spr[t], &cv, 255, 70, 200); break;
    case MK_ANTITARGET:      /* circle with a cross, four legs outward (axes) */
        ring(&cv, c, c, R, W);
        for (int i = 0; i < 4; i++) { tick(&cv, c, c, 45 + 90 * i, 0, R * 0.72f, W); tick(&cv, c, c, 90 * i, R, R + 10, W); }
        finish(&spr[t], &cv, 255, 70, 200); break;
    case MK_MANEUVER:        /* blue circle, dot, three wide legs */
        ring(&cv, c, c, R, W + 1); dot(&cv, c, c, 4);
        tick(&cv, c, c, 0, R, R + 12, W + 2); tick(&cv, c, c, 270, R, R + 12, W + 2); tick(&cv, c, c, 90, R, R + 12, W + 2);
        finish(&spr[t], &cv, 60, 120, 255); break;
    case MK_LEVEL: {         /* fixed vessel symbol: wings and a "W" in the middle */
        float m = LW / 2.0f, y = 12;
        line(&cv, m - 52, y, m - 22, y, 5); line(&cv, m + 22, y, m + 52, y, 5);
        line(&cv, m - 22, y, m - 11, y + 16, 5); line(&cv, m - 11, y + 16, m, y, 5);
        line(&cv, m, y, m + 11, y + 16, 5); line(&cv, m + 11, y + 16, m + 22, y, 5);
        dot(&cv, m, y, 3);
        finish(&spr[t], &cv, 255, 165, 0); break;
    }
    default: break;
    }
    free(cv.cov); free(cv.dark);
}

void hud_init(void)
{
    for (int t = 0; t < MK_COUNT; t++) build((marker_type_t)t);
}

void hud_sprite_size(marker_type_t t, int *w, int *h) { *w = spr[t].w; *h = spr[t].h; }

/* a = 1..256. The weight is reduced to 5 bits so the red / blue products packed in one word
 * cannot overlap (blue * 32 stays below bit 11). */
static inline uint16_t blend565(uint16_t d, uint16_t s, unsigned a)
{
    unsigned w = (a + 4) >> 3;                       /* 0..32 */
    unsigned rb = (((s & 0xF81F) * w + (d & 0xF81F) * (32 - w)) >> 5) & 0xF81F;
    unsigned g = (((s & 0x07E0) * w + (d & 0x07E0) * (32 - w)) >> 5) & 0x07E0;
    return (uint16_t)(rb | g);
}

void hud_blit(const marker_draw_t *m, int n, uint16_t *dst, int y0, int y1, int stride, int cx, int cy, int r)
{
    for (int k = 0; k < n; k++) {
        const sprite_t *sp = &spr[m[k].type];
        if (!sp->col) continue;
        int ya = m[k].y > y0 ? m[k].y : y0, yb = m[k].y + sp->h - 1 < y1 ? m[k].y + sp->h - 1 : y1;
        for (int y = ya; y <= yb; y++) {
            const int sy = y - m[k].y, dy = y - cy;
            const int rem = r * r - dy * dy;
            if (rem < 0) continue;
            const uint8_t *al = sp->alpha + sy * sp->w;
            const uint16_t *co = sp->col + sy * sp->w;
            uint16_t *row = dst + (y - y0) * stride;
            for (int sx = 0; sx < sp->w; sx++) {
                unsigned a = al[sx];
                if (!a) continue;
                int x = m[k].x + sx, dx = x - cx;
                if (dx * dx > rem) continue;                  /* clip to the ball */
                row[x] = a >= 250 ? co[sx] : blend565(row[x], co[sx], a + 1);
            }
        }
    }
}

/* ---------- labels painted on the ball ---------- */
static int glyph_index(char c)
{
    for (int i = 0; i < NBF_N; i++) if (NBF_GLYPHS[i].c == c) return i;
    return -1;
}

void hud_label(label_draw_t *l, const char *txt, float ox, float oy, float ax, float ay, float bx, float by, uint8_t alpha)
{
    l->n = 0;
    float tw = 2 * NBF_PAD;
    for (const char *p = txt; *p && l->n < 4; p++) {
        int g = glyph_index(*p);
        if (g < 0) continue;
        l->txt[l->n++] = *p;
        tw += NBF_GLYPHS[g].w - 2 * NBF_PAD;
    }
    l->txt[l->n] = 0;
    l->tw = tw;
    l->ox = ox; l->oy = oy; l->alpha = alpha;
    float det = ax * by - bx * ay;
    if (fabsf(det) < 1e-6f) { l->n = 0; return; }
    l->i00 = by / det;  l->i01 = -bx / det;
    l->i10 = -ay / det; l->i11 = ax / det;
    float hx = tw / 2, hy = NBF_H / 2.0f, minx = 1e9f, maxx = -1e9f, miny = 1e9f, maxy = -1e9f;
    for (int k = 0; k < 4; k++) {
        float u = (k & 1) ? hx : -hx, v = (k & 2) ? hy : -hy;
        float x = ox + u * ax + v * bx, y = oy + u * ay + v * by;
        minx = fminf(minx, x); maxx = fmaxf(maxx, x); miny = fminf(miny, y); maxy = fmaxf(maxy, y);
    }
    l->x0 = (int16_t)floorf(minx); l->x1 = (int16_t)ceilf(maxx);
    l->y0 = (int16_t)floorf(miny); l->y1 = (int16_t)ceilf(maxy);
}

static inline float sample(const uint8_t *img, float x, float y)   /* bilinear, 0..255 */
{
    if (x < 0 || y < 0 || x >= NBF_W - 1 || y >= NBF_H - 1) return 0;
    int xi = (int)x, yi = (int)y;
    float fx = x - xi, fy = y - yi;
    const uint8_t *p = img + yi * NBF_W + xi;
    float a = p[0] + (p[1] - p[0]) * fx, b = p[NBF_W] + (p[NBF_W + 1] - p[NBF_W]) * fx;
    return a + (b - a) * fy;
}

void hud_blit_labels(const label_draw_t *l, int n, uint16_t *dst, int y0, int y1, int stride, int cx, int cy, int r)
{
    const uint16_t white = rgb565(245, 245, 245), dark = rgb565(15, 20, 30);
    for (int k = 0; k < n; k++) {
        const label_draw_t *L = &l[k];
        if (!L->n) continue;
        int ya = L->y0 > y0 ? L->y0 : y0, yb = L->y1 < y1 ? L->y1 : y1;
        /* glyph positions along the text (core of each glyph, the padding overlaps) */
        int gi[4]; float gx[4];
        float pos = 0;
        for (int c = 0; c < L->n; c++) {
            gi[c] = glyph_index(L->txt[c]);
            gx[c] = pos;
            pos += NBF_GLYPHS[gi[c]].w - 2 * NBF_PAD;
        }
        for (int y = ya; y <= yb; y++) {
            const int dy = y - cy, rem = r * r - dy * dy;
            if (rem < 0) continue;
            uint16_t *row = dst + (y - y0) * stride;
            float ddy = y + 0.5f - L->oy, ddx = L->x0 + 0.5f - L->ox;
            float u = L->i00 * ddx + L->i01 * ddy + L->tw / 2;      /* glyph coords, origin top-left */
            float v = L->i10 * ddx + L->i11 * ddy + NBF_H / 2.0f;
            for (int x = L->x0; x <= L->x1; x++, u += L->i00, v += L->i10) {
                if (u < 0 || v < 0 || u >= L->tw || v >= NBF_H) continue;
                const int dx = x - cx;
                if (dx * dx > rem) continue;
                /* glyph under u: the last one whose start is before u - pad */
                int c = 0;
                while (c + 1 < L->n && u - NBF_PAD >= gx[c + 1]) c++;
                float sx = NBF_GLYPHS[gi[c]].x + (u - gx[c]);
                float e = sample(NBF_EDGE, sx, v), f = sample(NBF_FILL, sx, v);
                /* neighbour glyph overlapping through the padding */
                if (c + 1 < L->n && u - gx[c + 1] >= 0) {
                    float sx2 = NBF_GLYPHS[gi[c + 1]].x + (u - gx[c + 1]);
                    e = fmaxf(e, sample(NBF_EDGE, sx2, v)); f = fmaxf(f, sample(NBF_FILL, sx2, v));
                }
                if (e < 1 && f < 1) continue;
                unsigned ea = (unsigned)(e * L->alpha * 0.7f) >> 8, fa = (unsigned)(f * L->alpha) >> 8;
                uint16_t px = row[x];
                if (ea) px = blend565(px, dark, ea + 1);
                if (fa) px = blend565(px, white, fa + 1);
                row[x] = px;
            }
        }
    }
}

/* heading numbers along the horizon (every 30 deg, N / E / S / W) and pitch numbers (+-30, +-60)
 * along the four cardinal meridians, for the attitude M (camera -> navball frame) */
int hud_navball_labels(const float M[9], int cx, int cy, int r, label_draw_t *out)
{
    const float size = 0.058f;                       /* text height on the unit sphere */
    const float k = size / NBF_H, deg = (float)M_PI / 180;
    const float R[3] = {M[0], M[3], M[6]}, T[3] = {M[1], M[4], M[7]}, F[3] = {M[2], M[5], M[8]};
    int n = 0;
    for (int pass = 0; pass < 2; pass++) {
        for (int i = 0; i < (pass ? 16 : 12); i++) {
            char txt[6];
            float h, p;
            if (!pass) {                             /* heading */
                h = i * 30.0f; p = 3.6f;
                static const char *card[4] = {"N", "E", "S", "W"};
                if (i % 3 == 0) snprintf(txt, sizeof txt, "%s", card[i / 3]);
                else snprintf(txt, sizeof txt, "%d", i * 30);
            } else {                                 /* pitch */
                static const int pv[4] = {-60, -30, 30, 60};
                int pitch = pv[i % 4];
                p = pitch + 3.2f;
                h = (i / 4) * 90.0f + 6.0f / cosf(pitch * deg);
                snprintf(txt, sizeof txt, "%d", pitch);
            }
            const float sh = sinf(h * deg), ch = cosf(h * deg), sp = sinf(p * deg), cp = cosf(p * deg);
            const float P[3] = {sh * cp, sp, ch * cp};
            const float E[3] = {ch, 0, -sh};                     /* along the text: increasing heading */
            const float D[3] = {sh * sp, -cp, ch * sp};          /* down the text: decreasing pitch */
            float z = P[0] * F[0] + P[1] * F[1] + P[2] * F[2];
            if (z < 0.25f) continue;                             /* back side / too close to the rim */
            float a = (z - 0.25f) / 0.15f;
            uint8_t alpha = (uint8_t)(255 * (a > 1 ? 1 : a));
            float sx = cx + r * (P[0] * R[0] + P[1] * R[1] + P[2] * R[2]);
            float sy = cy - r * (P[0] * T[0] + P[1] * T[1] + P[2] * T[2]);
            float ax = r * k * (E[0] * R[0] + E[1] * R[1] + E[2] * R[2]), ay = -r * k * (E[0] * T[0] + E[1] * T[1] + E[2] * T[2]);
            float bx = r * k * (D[0] * R[0] + D[1] * R[1] + D[2] * R[2]), by = -r * k * (D[0] * T[0] + D[1] * T[1] + D[2] * T[2]);
            hud_label(&out[n], txt, sx, sy, ax, ay, bx, by, alpha);
            if (out[n].n) n++;
        }
    }
    return n;
}
