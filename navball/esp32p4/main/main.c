/* Navball on the Waveshare ESP32-P4-WIFI6-Touch-LCD-3.4C (round MIPI-DSI panel, 720x720 in practice).
 *
 * Port of navball/rp2040/main.c:
 *  - same renderer (navball.c), split over the two P4 cores like the RP2040 used core0/core1,
 *  - RENDER_DIV 1 (default): full resolution, each core renders 16-row strips in internal SRAM
 *    (SIMD kernel, navball_simd.S) that a DMA copies into the PSRAM frame buffer:
 *    16.5 ms per frame at 720x720 (60 fps max), capped at TARGET_FPS = 30 -> ~50 % of the CPU free,
 *    RENDER_DIV 2: half resolution in internal SRAM, enlarged x2 by the PPA (~44 fps),
 *  - KSP-style navball: attitude (heading, pitch, roll) and markers (prograde, normal, radial,
 *    target, maneuver and their opposites), received as text lines on both USB ports (USB-UART
 *    bridge = UART0, native USB = USB-Serial-JTAG, the one wired to the Eeloo Pad hub), see
 *    PROTOCOL below and tools/navball_sim.py,
 *  - double or triple buffering in the DPI frame buffers (CONFIG_BSP_LCD_DPI_BUFFER_NUMS), swapped at the end of a refresh,
 *  - demo animation when no attitude was received for 2 s,
 *  - no render while the attitude does not change; FPS / timing / load printed every 2 s.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_cache.h"
#include "esp_heap_caps.h"
#include "esp_memory_utils.h"
#include "esp_async_memcpy.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_mipi_dsi.h"
#include "driver/ppa.h"
#include "driver/uart.h"
#include "driver/usb_serial_jtag.h"
#include "driver/usb_serial_jtag_vfs.h"
#include "bsp/esp-bsp.h"
#include "bsp/display.h"
#include <ctype.h>
#include "navball.h"
#include "hud.h"

static const char *TAG = "navball";

#define W BSP_LCD_H_RES                 /* panel resolution */
#define H BSP_LCD_V_RES
#define RENDER_DIV 1                    /* 1 = render at full resolution, 2 = half and scale x2 */
#define NFB CONFIG_BSP_LCD_DPI_BUFFER_NUMS  /* DPI frame buffers: 2 = double, 3 = triple buffering */
#define RW (W / RENDER_DIV)             /* render buffer resolution */
#define RH (H / RENDER_DIV)
#define DEMO_TIMEOUT_US (2 * 1000 * 1000)
#define TARGET_FPS 30                   /* frame rate cap, the rest is margin (0 = as fast as possible) */

static esp_lcd_panel_handle_t panel;
static uint16_t *fbs[NFB];              /* DPI frame buffers (PSRAM, W x H) */
static uint16_t *rbuf;                  /* render buffer (internal SRAM, RW x RH) */
#if RENDER_DIV > 1
static ppa_client_handle_t ppa;
#endif
static SemaphoreHandle_t vsync_sem;
static navball_ctx_t ctx;

/* ---------- navball state received from the PC ----------
 * PROTOCOL: one text line per item, degrees, '.' decimal separator, any order, at any rate:
 *   A <heading> <pitch> <roll>     vessel attitude (heading from north to east, pitch above the
 *                                  horizon, roll positive = right wing down)
 *   P <heading> <pitch>            prograde      (retrograde is drawn opposite)
 *   N <heading> <pitch>            orbit normal  (anti-normal opposite)
 *   R <heading> <pitch>            radial out    (radial in opposite)
 *   T <heading> <pitch>            target        (anti-target opposite)
 *   M <heading> <pitch>            next maneuver
 *   P - / N - / R - / T - / M -    hide that marker (e.g. no target, no maneuver node)
 *   <h> <p> <r>                    three bare numbers = attitude (old RP2040 test format)
 * Marker directions are in the same local frame as the attitude (north / east / up).
 * BINARY frames (sent by the KSP plugin plugin/KSP1_Serial, can be mixed with text lines):
 *   0xEE 0x5A | type | len | payload | crc8 (poly 0x07 over type, len, payload)
 *   type 0x01 NAVBALL, 97 bytes: u8 flags (bit0..4 = prograde, normal, radial, target, maneuver
 *   valid, bits 5-6 speed mode), then 8 x float3 little-endian: right, top, nose (vessel axes),
 *   prograde, normal, radial out, target, maneuver; all unit vectors in the frame x = east,
 *   y = up, z = north (= the navball frame), so the vessel axes are directly the matrix M.
 * Without any data for 2 s the navball runs a demo. */
enum { D_PRO, D_NRM, D_RAD, D_TGT, D_MNV, D_COUNT };
typedef struct {
    float M[9];                         /* camera -> navball frame (columns: right, top, nose) */
    float d[D_COUNT][3];                /* marker directions, navball frame */
    uint8_t on;                         /* bit i = marker i shown */
    uint8_t mode;                       /* navball speed mode (0 orbit, 1 surface, 2 target) */
} navstate_t;


static navstate_t g_state;              /* written by the RX tasks, read by the render loop */
static portMUX_TYPE g_state_lock = portMUX_INITIALIZER_UNLOCKED;
static volatile int64_t att_last_us = -DEMO_TIMEOUT_US;
static SemaphoreHandle_t att_sem;       /* given at every accepted line */

static int parse_floats(char *s, float *v, int n)
{
    char *e;
    for (int i = 0; i < n; i++) {
        v[i] = strtof(s, &e);
        if (e == s) return 0;
        s = e;
    }
    return 1;
}

static void parse_line(char *s)
{
    while (*s == ' ' || *s == '\t') s++;
    float v[3] = {0, 0, 0};
    navstate_t upd;
    int item;                                   /* -1 = attitude, else D_* */
    bool on = true;
    if (isalpha((unsigned char)s[0])) {
        char cmd = (char)toupper((unsigned char)s[0]);
        s++;
        if (cmd == 'A') {
            if (!parse_floats(s, v, 3)) return;
            item = -1;
        } else {
            const char *k = strchr("PNRTM", cmd);
            if (!k) return;
            item = (int)(k - "PNRTM");
            while (*s == ' ' || *s == '\t') s++;
            if (*s == '-' && (s[1] == 0 || s[1] == ' ')) on = false;
            else if (!parse_floats(s, v, 2)) return;
        }
    } else {
        if (!parse_floats(s, v, 3)) return;    /* old format: three numbers = attitude */
        item = -1;
    }
    float M[9], d[3];
    if (item < 0) navball_attitude_matrix(v[0], v[1], v[2], M);
    else if (on) navball_direction(v[0], v[1], d);
    taskENTER_CRITICAL(&g_state_lock);
    upd = g_state;
    if (item < 0) memcpy(upd.M, M, sizeof M);
    else if (on) { memcpy(upd.d[item], d, sizeof d); upd.on |= 1 << item; }
    else upd.on &= ~(1 << item);
    g_state = upd;
    taskEXIT_CRITICAL(&g_state_lock);
    att_last_us = esp_timer_get_time();
    xSemaphoreGive(att_sem);
}

/* ---------- binary frames (KSP plugin) ---------- */
#define SYNC0 0xEE
#define SYNC1 0x5A
#define TYPE_NAVBALL 0x01
static volatile uint32_t rx_frames, rx_crc_errors;

static uint8_t crc8(const uint8_t *p, int n, uint8_t crc)
{
    while (n--) {
        crc ^= *p++;
        for (int b = 0; b < 8; b++) crc = (crc & 0x80) ? (uint8_t)((crc << 1) ^ 0x07) : (uint8_t)(crc << 1);
    }
    return crc;
}

static void handle_frame(uint8_t type, const uint8_t *p, int len)
{
    if (type != TYPE_NAVBALL || len != 97) return;
    float v[24];
    memcpy(v, p + 1, sizeof v);                 /* little-endian floats, same as the P4 */
    navstate_t upd;
    for (int i = 0; i < 3; i++) {               /* columns of M: right, top, nose */
        upd.M[3 * i + 0] = v[0 + i];
        upd.M[3 * i + 1] = v[3 + i];
        upd.M[3 * i + 2] = v[6 + i];
    }
    for (int k = 0; k < D_COUNT; k++) memcpy(upd.d[k], &v[9 + 3 * k], 3 * sizeof(float));
    upd.on = p[0] & 0x1F;
    upd.mode = (p[0] >> 5) & 3;
    taskENTER_CRITICAL(&g_state_lock);
    g_state = upd;
    taskEXIT_CRITICAL(&g_state_lock);
    rx_frames++;
    att_last_us = esp_timer_get_time();
    xSemaphoreGive(att_sem);
}

/* per-port receiver: text lines and binary frames can be mixed (0xEE never appears in text) */
typedef struct {
    char buf[64]; int n;                        /* text line */
    uint8_t st, type, len, idx;                 /* binary frame state */
    uint8_t payload[255];
} line_t;

static void feed(line_t *l, const uint8_t *d, int len)
{
    for (int i = 0; i < len; i++) {
        uint8_t c = d[i];
        switch (l->st) {
        case 0:
            if (c == SYNC0) { l->st = 1; break; }
            if (c == '\n' || c == '\r') {
                if (l->n) { l->buf[l->n] = 0; parse_line(l->buf); l->n = 0; }
            } else if (l->n < (int)sizeof(l->buf) - 1) {
                l->buf[l->n++] = (char)c;
            } else {
                l->n = 0;                       /* garbage, drop */
            }
            break;
        case 1: l->st = c == SYNC1 ? 2 : 0; break;
        case 2: l->type = c; l->st = 3; break;
        case 3: l->len = c; l->idx = 0; l->st = c ? 4 : 5; break;
        case 4: l->payload[l->idx++] = c; if (l->idx == l->len) l->st = 5; break;
        case 5: {
            uint8_t hdr[2] = {l->type, l->len};
            uint8_t crc = crc8(l->payload, l->len, crc8(hdr, 2, 0));
            if (crc == c) handle_frame(l->type, l->payload, l->len);
            else rx_crc_errors++;
            l->st = 0;
            break;
        }
        }
    }
}

/* Reception is interrupt driven: each task sleeps until bytes arrive (no polling). */
static QueueHandle_t uart_queue;

static void uart_rx_task(void *arg)       /* UART0 = USB-UART Type-C port (also the console) */
{
    line_t l = {0};
    uint8_t d[128];
    uart_event_t ev;
    while (1) {
        if (!xQueueReceive(uart_queue, &ev, portMAX_DELAY)) continue;
        if (ev.type == UART_DATA) {
            int n;
            while ((n = uart_read_bytes(UART_NUM_0, d, sizeof d, 0)) > 0) feed(&l, d, n);
        } else if (ev.type == UART_FIFO_OVF || ev.type == UART_BUFFER_FULL) {
            uart_flush_input(UART_NUM_0);
            xQueueReset(uart_queue);
            l.n = 0;
        }
    }
}

static void jtag_rx_task(void *arg)       /* USB-Serial-JTAG = USB Full Speed Type-C port */
{
    line_t l = {0};
    uint8_t d[64];
    while (1) {
        int n = usb_serial_jtag_read_bytes(d, sizeof d, portMAX_DELAY);   /* returns on any data */
        if (n > 0) feed(&l, d, n);
    }
}

static void serial_init(void)
{
    att_sem = xSemaphoreCreateBinary();
    navball_attitude_matrix(0, 0, 0, g_state.M);   /* valid attitude until the first one arrives */
    ESP_ERROR_CHECK(uart_driver_install(UART_NUM_0, 1024, 0, 16, &uart_queue, 0));
    xTaskCreate(uart_rx_task, "uart_rx", 3072, NULL, 6, NULL);
    usb_serial_jtag_driver_config_t jcfg = USB_SERIAL_JTAG_DRIVER_CONFIG_DEFAULT();
    if (usb_serial_jtag_driver_install(&jcfg) == ESP_OK) {
        usb_serial_jtag_vfs_use_driver();
        xTaskCreate(jtag_rx_task, "jtag_rx", 3072, NULL, 6, NULL);
    }
}

/* ---------- rendering on both cores ---------- */
static TaskHandle_t main_task, worker_task;
static volatile int band_y0, band_y1;
static volatile int64_t worker_us;
static marker_draw_t g_draw[12];         /* markers of the frame being rendered */
static int g_ndraw;
static label_draw_t g_labels[HUD_MAX_LABELS];   /* heading / pitch numbers of the frame */
static int g_nlabels;

#if RENDER_DIV == 1
/* Full resolution: each core renders its band as strips of STRIP_ROWS rows into a small buffer in
 * internal SRAM (fast writes), and a DMA copies each strip into the PSRAM frame buffer while the
 * core renders the next one (two strip buffers per core). Writing the frame buffer directly from
 * the CPU costs ~15 % more: every cache line written is first read from PSRAM. */
#define STRIP_ROWS 16                   /* 16 rows x 1440 B = 23040 B, a multiple of the cache line */
typedef struct {
    uint16_t *buf[2];
    SemaphoreHandle_t done[2];
    bool busy[2];
    async_memcpy_handle_t mcp;
} strips_t;
static strips_t strips[2];              /* one set per core */

static bool IRAM_ATTR strip_copied(async_memcpy_handle_t h, async_memcpy_event_t *e, void *arg)
{
    BaseType_t w = pdFALSE;
    xSemaphoreGiveFromISR((SemaphoreHandle_t)arg, &w);
    return w == pdTRUE;
}

static void strips_init(void)
{
    async_memcpy_config_t cfg = ASYNC_MEMCPY_DEFAULT_CONFIG();
    for (int c = 0; c < 2; c++) {
        ESP_ERROR_CHECK(esp_async_memcpy_install_gdma_axi(&cfg, &strips[c].mcp));
        for (int k = 0; k < 2; k++) {
            strips[c].buf[k] = heap_caps_aligned_calloc(128, 1, STRIP_ROWS * W * 2, MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA);
            assert(strips[c].buf[k]);
            strips[c].done[k] = xSemaphoreCreateBinary();
        }
    }
}

/* rows [y0, y1] (full rows, y0 multiple of STRIP_ROWS) into the frame buffer ctx.fb */
static void render_band(int y0, int y1)
{
    strips_t *st = &strips[xPortGetCoreID()];
    int k = 0;
    for (int y = y0; y <= y1; y += STRIP_ROWS, k ^= 1) {
        int n = y1 - y + 1 < STRIP_ROWS ? y1 - y + 1 : STRIP_ROWS;
        if (st->busy[k]) { xSemaphoreTake(st->done[k], portMAX_DELAY); st->busy[k] = false; }
        navball_render_rows_to(&ctx, y, y + n - 1, st->buf[k], W, 1);
        hud_blit_labels(g_labels, g_nlabels, st->buf[k], y, y + n - 1, W, ctx.cx, ctx.cy, ctx.radius_px);
        hud_blit(g_draw, g_ndraw, st->buf[k], y, y + n - 1, W, ctx.cx, ctx.cy, ctx.radius_px);
        esp_cache_msync(st->buf[k], n * W * 2, ESP_CACHE_MSYNC_FLAG_DIR_C2M | ESP_CACHE_MSYNC_FLAG_UNALIGNED);
        ESP_ERROR_CHECK(esp_async_memcpy(st->mcp, ctx.fb + y * W, st->buf[k], n * W * 2, strip_copied, st->done[k]));
        st->busy[k] = true;
    }
    for (k = 0; k < 2; k++)
        if (st->busy[k]) { xSemaphoreTake(st->done[k], portMAX_DELAY); st->busy[k] = false; }
}
#else
/* Each band is written back from the core that drew it (each P4 core has its own L1 cache).
 * The split row is a multiple of 16 rows so no cache line is shared between the two bands. */
static void render_band(int y0, int y1)
{
    if (y0 < ctx.cy - ctx.radius_px) y0 = ctx.cy - ctx.radius_px;
    if (y1 > ctx.cy + ctx.radius_px) y1 = ctx.cy + ctx.radius_px;
    navball_render_rows(&ctx, y0, y1);
    esp_cache_msync(ctx.fb + y0 * ctx.stride, (size_t)(y1 - y0 + 1) * ctx.stride * 2,
                    ESP_CACHE_MSYNC_FLAG_DIR_C2M | ESP_CACHE_MSYNC_FLAG_UNALIGNED);
}
#endif

static void render_worker(void *arg)
{
    while (1) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        int64_t t0 = esp_timer_get_time();
        render_band(band_y0, band_y1);           /* top band */
        worker_us = esp_timer_get_time() - t0;
        xTaskNotifyGive(main_task);
    }
}

/* Buffer swap: draw_bitmap() only records the new buffer index; the DPI driver applies it when
 * it restarts the DMA at the end of the frame being scanned, then calls this callback.
 * swap_pending is raised only after draw_bitmap() returned, so the first callback that sees it
 * comes after a restart on the new buffer: the old buffer is then free. */
static volatile bool swap_pending;

static bool IRAM_ATTR on_frame_done(esp_lcd_panel_handle_t p, esp_lcd_dpi_panel_event_data_t *e, void *u)
{
    BaseType_t woken = pdFALSE;
    if (swap_pending) {
        swap_pending = false;
        xSemaphoreGiveFromISR(vsync_sem, &woken);
    }
    return woken == pdTRUE;
}

static void display_init(void)
{
    esp_lcd_panel_io_handle_t io = NULL;
    ESP_ERROR_CHECK(bsp_display_new(NULL, &panel, &io));
    esp_lcd_panel_disp_on_off(panel, true);
#if NFB == 3
    ESP_ERROR_CHECK(esp_lcd_dpi_panel_get_frame_buffer(panel, 3, (void **)&fbs[0], (void **)&fbs[1], (void **)&fbs[2]));
#else
    ESP_ERROR_CHECK(esp_lcd_dpi_panel_get_frame_buffer(panel, 2, (void **)&fbs[0], (void **)&fbs[1]));
#endif
    for (int i = NFB - 1; i >= 0; i--) {                     /* black outside the ball, written back */
        memset(fbs[i], 0, W * H * 2);
        esp_lcd_panel_draw_bitmap(panel, 0, 0, W, H, fbs[i]);
    }

    vsync_sem = xSemaphoreCreateBinary();
    esp_lcd_dpi_panel_event_callbacks_t cbs = {.on_refresh_done = on_frame_done};
    ESP_ERROR_CHECK(esp_lcd_dpi_panel_register_event_callbacks(panel, &cbs, NULL));
    bsp_display_backlight_on();

#if RENDER_DIV > 1
    /* render buffer in internal SRAM when it fits (faster than PSRAM), black outside the ball */
    size_t sz = RW * RH * 2;
    rbuf = heap_caps_aligned_calloc(128, 1, sz, MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA);
    if (!rbuf) rbuf = heap_caps_aligned_calloc(128, 1, sz, MALLOC_CAP_SPIRAM | MALLOC_CAP_DMA);
    assert(rbuf);
    esp_cache_msync(rbuf, sz, ESP_CACHE_MSYNC_FLAG_DIR_C2M | ESP_CACHE_MSYNC_FLAG_UNALIGNED);
    ppa_client_config_t pc = {.oper_type = PPA_OPERATION_SRM, .max_pending_trans_num = 1};
    ESP_ERROR_CHECK(ppa_register_client(&pc, &ppa));
    ESP_LOGI(TAG, "render %dx%d in %s, PPA x%d to %dx%d", RW, RH,
             esp_ptr_internal(rbuf) ? "internal SRAM" : "PSRAM", RENDER_DIV, W, H);
#endif
}

/* enlarge the render buffer into a display frame buffer with the PPA */
static void scale_to(uint16_t *fb)
{
#if RENDER_DIV > 1
    ppa_srm_oper_config_t op = {
        .in = {.buffer = rbuf, .pic_w = RW, .pic_h = RH, .block_w = RW, .block_h = RH,
               .block_offset_x = 0, .block_offset_y = 0, .srm_cm = PPA_SRM_COLOR_MODE_RGB565},
        .out = {.buffer = fb, .buffer_size = W * H * 2, .pic_w = W, .pic_h = H,
                .block_offset_x = 0, .block_offset_y = 0, .srm_cm = PPA_SRM_COLOR_MODE_RGB565},
        .rotation_angle = PPA_SRM_ROTATION_ANGLE_0,
        .scale_x = RENDER_DIV, .scale_y = RENDER_DIV,
        .mode = PPA_TRANS_MODE_BLOCKING,
    };
    ESP_ERROR_CHECK(ppa_do_scale_rotate_mirror(ppa, &op));
#else
    (void)fb;
#endif
}

/* demo: a plausible scene so the markers can be seen without the PC */
static void demo_state(navstate_t *s, float t)
{
    static const float dirs[D_COUNT][2] = {{80, 5}, {20, 10}, {0, 80}, {115, -10}, {62, 24}};
    navball_attitude_matrix(90 + 35 * sinf(0.21f * t), 20 * sinf(0.13f * t), 25 * sinf(0.17f * t), s->M);
    for (int k = 0; k < D_COUNT; k++) navball_direction(dirs[k][0], dirs[k][1], s->d[k]);
    s->on = 0x1F;
}

/* project the markers of state s (attitude matrix M) to screen sprites, back side hidden */
static int project_markers(const navstate_t *s, marker_draw_t *out)
{
    static const uint8_t type[D_COUNT][2] = {
        {MK_PROGRADE, MK_RETROGRADE}, {MK_NORMAL, MK_ANTINORMAL}, {MK_RADIAL_OUT, MK_RADIAL_IN},
        {MK_TARGET, MK_ANTITARGET}, {MK_MANEUVER, MK_COUNT}};
    static const int order[D_COUNT] = {D_RAD, D_NRM, D_TGT, D_PRO, D_MNV};   /* drawn bottom to top */
    const int cx = RW / 2, cy = RH / 2, r = (RW - 2) / 2;
    int n = 0;
    for (int oi = 0; oi < D_COUNT; oi++) {
        const int i = order[oi];
        if (!(s->on & (1 << i))) continue;
        const float *d = s->d[i], *M = s->M;
        for (int sign = 0; sign < 2; sign++) {
            const int t = type[i][sign];
            if (t == MK_COUNT) continue;
            const float k = sign ? -1.0f : 1.0f;
            /* camera coordinates: v_cam = M^T d (columns of M = right, up, nose) */
            float vx = k * (M[0] * d[0] + M[3] * d[1] + M[6] * d[2]);
            float vy = k * (M[1] * d[0] + M[4] * d[1] + M[7] * d[2]);
            float vz = k * (M[2] * d[0] + M[5] * d[1] + M[8] * d[2]);
            if (vz <= 0.03f) continue;                         /* behind the ball */
            int w, h;
            hud_sprite_size((marker_type_t)t, &w, &h);
            out[n++] = (marker_draw_t){(int16_t)(cx + vx * r - w / 2), (int16_t)(cy - vy * r - h / 2), (uint8_t)t};
        }
    }
    int w, h;                                                  /* fixed vessel symbol, on top */
    hud_sprite_size(MK_LEVEL, &w, &h);
    out[n++] = (marker_draw_t){(int16_t)(cx - w / 2), (int16_t)(cy - 12), MK_LEVEL};
    return n;
}

void app_main(void)
{
    ESP_LOGI(TAG, "navball ESP32-P4, panel %dx%d", W, H);
    navball_init();
    hud_init();
    display_init();
#if RENDER_DIV == 1
    strips_init();
#endif

    {   /* check the SIMD kernel against its C reference before using it */
        navball_ctx_t t;
        float Mt[9];
        navball_attitude_matrix(33.3f, 21.7f, 48.2f, Mt);
        navball_prepare(&t, fbs[0], RW, RH, RW, Mt, 1);
        int bad = navball_simd_selftest(&t);
        if (bad) ESP_LOGE(TAG, "SIMD self-test: %d pixels differ from the C reference", bad);
        else ESP_LOGI(TAG, "SIMD self-test passed");
    }
    main_task = xTaskGetCurrentTaskHandle();
    xTaskCreatePinnedToCore(render_worker, "render1", 4096, NULL, 5, &worker_task, 1);
    serial_init();
    vTaskPrioritySet(NULL, 5);

    /* triple buffering: 'shown' is on screen (or being replaced), 'pending' was handed to the
     * driver and becomes visible at the next frame end; we always draw into the third one. */
    int shown = 0, pending = -1;
    int split = 0;
    int frames = 0;
    navstate_t last;
    memset(&last, 0xFF, sizeof last);
    int64_t t_stat = esp_timer_get_time(), render_acc = 0, core1_acc = 0, ppa_acc = 0, frame_acc = 0;
    int64_t next_frame = t_stat;

    while (1) {
        /* frame pacing: at most TARGET_FPS frames per second, the remaining time is free */
#if TARGET_FPS
        int64_t wait = next_frame - esp_timer_get_time();
        if (wait >= 1000) vTaskDelay(pdMS_TO_TICKS(wait / 1000));
#endif
        int64_t t_frame = esp_timer_get_time();
#if TARGET_FPS
        next_frame = (next_frame > t_frame ? next_frame : t_frame) + 1000000 / TARGET_FPS;
#endif
        navstate_t st;
        bool demo = t_frame - att_last_us > DEMO_TIMEOUT_US;
        if (demo) {
            memset(&st, 0, sizeof st);
            demo_state(&st, t_frame / 1e6f);
        } else {
            taskENTER_CRITICAL(&g_state_lock);
            st = g_state;
            taskEXIT_CRITICAL(&g_state_lock);
        }
        if (memcmp(&st, &last, sizeof st) == 0) {
            /* nothing changed: no render, wait for the next attitude (or the demo timeout) */
            xSemaphoreTake(att_sem, pdMS_TO_TICKS(50));
            next_frame = 0;                        /* render the next change at once */
        } else {
            last = st;

            if (NFB == 2 && pending >= 0) {            /* double buffering: no free buffer until the swap */
                xSemaphoreTake(vsync_sem, pdMS_TO_TICKS(100));
                shown = pending; pending = -1;
            }
            int back = 0;
            while (back == shown || back == pending) back++;

            /* 1. render the ball on both cores */
            uint16_t *target = RENDER_DIV > 1 ? rbuf : fbs[back];
            navball_prepare(&ctx, target, RW, RH, RW, st.M, 1);
            g_ndraw = RENDER_DIV == 1 ? project_markers(&st, g_draw) : 0;   /* markers: full resolution only */
            g_nlabels = RENDER_DIV == 1 ? hud_navball_labels(st.M, RW / 2, RH / 2, (RW - 2) / 2, g_labels) : 0;
            int y0 = 0, y1 = RH - 1;                   /* full rows; the bands clip/clear as needed */
            /* split row, multiple of 16 (no shared cache line, strip aligned), moved so both cores
             * finish together (core 0 also serves the interrupts) */
            int mid = ((RH / 2) + split) & ~15;
            band_y0 = y0; band_y1 = mid - 1;
            xTaskNotifyGive(worker_task);              /* core 1: top band */
            int64_t t_b = esp_timer_get_time();
            render_band(mid, y1);                      /* core 0: bottom band */
            int64_t core0_us = esp_timer_get_time() - t_b;
            ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
            if (core0_us > worker_us + 800 && split < 160) split += 16;
            else if (worker_us > core0_us + 800 && split > -160) split -= 16;
            int64_t t_render = esp_timer_get_time();

            /* 2. scale into the back frame buffer (PPA) */
            scale_to(fbs[back]);
            int64_t t_ppa = esp_timer_get_time();

            /* 3. the previous frame must be on screen before we queue this one (it frees 'shown') */
            if (pending >= 0) {
                xSemaphoreTake(vsync_sem, pdMS_TO_TICKS(100));
                shown = pending;
            }
            esp_lcd_panel_draw_bitmap(panel, 0, 0, W, H, fbs[back]);   /* cache write-back + select buffer */
            swap_pending = true;
            pending = back;

            int64_t now = esp_timer_get_time();
            render_acc += t_render - t_frame; core1_acc += worker_us; ppa_acc += t_ppa - t_render; frame_acc += now - t_frame;
            frames++;
        }   /* end of 'attitude changed' */

        int64_t now = esp_timer_get_time();
        if (now - t_stat > 2000000) {
            float dt = (now - t_stat) / 1e6f, n = frames ? frames : 1;
            /* load = share of the time the cores spend rendering: 100 % - load is free */
            ESP_LOGI(TAG, "%.1f fps (cap %d) | render %.1f ms/frame (core1 %.1f, split %+d) + PPA %.1f | load %.0f %% | %s (nose hdg %.1f pitch %.1f, %d sprites, %d labels) | rx frames %u, crc err %u",
                     frames / dt, TARGET_FPS, render_acc / 1e3f / n, core1_acc / 1e3f / n, split, ppa_acc / 1e3f / n,
                     (render_acc + ppa_acc) / 1e4f / dt, demo ? "demo" : "serial",
                     fmodf(atan2f(last.M[2], last.M[8]) * 57.29578f + 360.0f, 360.0f), asinf(fminf(1, fmaxf(-1, last.M[5]))) * 57.29578f,
                     g_ndraw, g_nlabels, (unsigned)rx_frames, (unsigned)rx_crc_errors);
            frames = 0; render_acc = core1_acc = ppa_acc = frame_acc = 0; t_stat = now;
        }
    }
}
