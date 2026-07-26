#include "ac_brand_manager.h"
#include <string.h>

static struct {
    module_t *mod;
    const brand_config_t *configs[MAX_BRANDS];
    brand_t *ctxs[MAX_BRANDS]; 
    uint8_t count, index, locked;
    const brand_config_t *active;
} g_bm;

/* 前向声明 */
static const event_handler_t scan_table;
static void update_phy_rx(void);

static void scan_nop2(void *c, uint8_t a, uint8_t b) { (void)c;(void)a;(void)b; }
static void scan_nop1(void *c) { (void)c; }
static int  scan_nop_rx(void *c, uint8_t *d, uint16_t n) { (void)c;(void)d;(void)n; return 0; }

static void scan_periodic(void *ctx) {
    if (g_bm.locked || !g_bm.active) return;
    if (g_bm.active->evt_table->on_periodic_send)
        g_bm.active->evt_table->on_periodic_send(ctx);
}

static int scan_rx_frame(void *ctx, uint8_t *data, uint16_t len) {
    if (g_bm.locked || !g_bm.active) return 0;
    if (g_bm.active->evt_table->on_rx_frame &&
        g_bm.active->evt_table->on_rx_frame(ctx, data, len)) {
        g_bm.locked = 1;
        g_bm.mod->handler = g_bm.active->evt_table;
        g_bm.mod->handler_ctx = ctx;
        update_phy_rx();
        return 1;
    }
    uint8_t next = (g_bm.index + 1) % g_bm.count;
    g_bm.index = next;
    g_bm.active = g_bm.configs[next];
    g_bm.mod->handler_ctx = g_bm.ctxs[next];
    update_phy_rx();
    if (g_bm.active->evt_table->on_scan)
        g_bm.active->evt_table->on_scan(g_bm.ctxs[next]);
    return 0;
}

static void scan_start(void *ctx) {
    (void)ctx;
    g_bm.locked = 0; g_bm.index = 0;
    g_bm.active = g_bm.configs[0];
    g_bm.mod->handler = (event_handler_t*)&scan_table;
    g_bm.mod->handler_ctx = g_bm.ctxs[0];
    if (g_bm.active->evt_table->on_scan)
        g_bm.active->evt_table->on_scan(g_bm.ctxs[0]);
}

static const event_handler_t scan_table = {
    .on_periodic_send = scan_periodic, .on_rx_frame = scan_rx_frame,
    .on_control_cmd = scan_nop2, .on_need_ack = scan_nop1,
    .on_timeout = scan_nop1, .on_scan = scan_start,
    .on_rx_byte = NULL, .on_rx_isr = NULL,
};

void ac_brand_manager_init(module_t *m) {
    memset(&g_bm, 0, sizeof(g_bm));
    g_bm.mod = m;
    g_bm.mod->handler = (event_handler_t*)&scan_table;
}

void ac_brand_manager_register(const brand_config_t *cfg, brand_t *ctx) {
    if (g_bm.count >= MAX_BRANDS) return;
    g_bm.configs[g_bm.count] = cfg;
    g_bm.ctxs[g_bm.count] = ctx;
    ctx->mod = g_bm.mod;
    g_bm.count++;
}

static void update_phy_rx(void) {
    const event_handler_t *h = g_bm.mod->handler;
    if (h && h->on_rx_byte)
        g_bm.mod->phy->set_receive_callback(g_bm.mod->phy, h->on_rx_byte, g_bm.mod->handler_ctx);
}

void ac_brand_manager_start_scan(void) {
    g_bm.locked = 0; g_bm.index = 0;
    g_bm.active = g_bm.configs[0];
    g_bm.mod->handler = (event_handler_t*)&scan_table;
    g_bm.mod->handler_ctx = g_bm.ctxs[0];
    update_phy_rx();
    if (g_bm.active->evt_table->on_scan)
        g_bm.active->evt_table->on_scan(g_bm.ctxs[0]);
}

void ac_brand_manager_lock(void) { g_bm.locked = 1; }
int  ac_brand_manager_locked(void) { return g_bm.locked; }
const brand_config_t *ac_brand_manager_current(void) { return g_bm.active; }
void ac_brand_manager_set_poll_period(uint16_t ms) { module_set_poll_period(g_bm.mod, ms); }
