#pragma once
#include <stdint.h>
#include <stdbool.h>

/* Navball markers (KSP style): drawn as anti-aliased sprites over the ball, in the render strips. */
typedef enum {
    MK_PROGRADE, MK_RETROGRADE,
    MK_NORMAL, MK_ANTINORMAL,
    MK_RADIAL_OUT, MK_RADIAL_IN,
    MK_TARGET, MK_ANTITARGET,
    MK_MANEUVER,
    MK_LEVEL,                   /* fixed vessel symbol at the centre */
    MK_COUNT
} marker_type_t;

typedef struct {
    int16_t x, y;               /* top-left of the sprite on screen */
    uint8_t type;
} marker_draw_t;

/* Text label painted on the ball (heading / pitch numbers): an affine map approximates the sphere
 * around the label, so the text turns and squashes with the ball like KSP's navball texture. */
typedef struct {
    float ox, oy;               /* screen position of the label centre */
    float i00, i01, i10, i11;   /* screen offset -> glyph pixels (inverse of the label's affine map) */
    float tw;                   /* text width in glyph pixels */
    int16_t x0, y0, x1, y1;     /* screen bounding box */
    uint8_t alpha;              /* fade near the rim, 0..255 */
    uint8_t n;
    char txt[5];
} label_draw_t;

/* builds a label: centre (ox, oy) on screen, A = screen vector of one glyph pixel along the
 * text line, B = screen vector of one glyph pixel downwards */
void hud_label(label_draw_t *l, const char *txt, float ox, float oy, float ax, float ay, float bx, float by, uint8_t alpha);
void hud_blit_labels(const label_draw_t *l, int n, uint16_t *dst, int y0, int y1, int stride, int cx, int cy, int r);

#define HUD_MAX_LABELS 28
/* heading / pitch numbers for the attitude M (camera -> navball frame); returns the count */
int hud_navball_labels(const float M[9], int cx, int cy, int r, label_draw_t *out);

void hud_init(void);            /* builds the sprites, call once */
/* sprite size of a marker type (for centring) */
void hud_sprite_size(marker_type_t t, int *w, int *h);
/* draw the markers into rows [y0, y1] of dst (row y0 at dst), clipped to the ball (cx, cy, r) */
void hud_blit(const marker_draw_t *m, int n, uint16_t *dst, int y0, int y1, int stride,
              int cx, int cy, int r);
