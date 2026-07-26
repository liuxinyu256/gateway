/**
 * third_party_mgr.c —— 第三方485管理器
 * 其他单片机通过RS485和网关通信
 */
#include "third_party_mgr.h"

static module_t        *g_mod;
static gateway_device_t *g_gw;

static void on_periodic(void *ctx) { (void)ctx; }
static int  on_rx_frame(void *ctx, uint8_t *d, uint16_t n) {
    (void)ctx;(void)d;(void)n; return 0;
}
static void on_control(void *ctx, uint8_t cmd, uint8_t val) {
    (void)ctx;(void)cmd;(void)val;
}

static const event_handler_t tp_handler = {
    .on_periodic_send = on_periodic,
    .on_rx_frame      = on_rx_frame,
    .on_control_cmd   = on_control,
};

void third_party_mgr_init(module_t *m, gateway_device_t *gw) {
    g_mod = m; g_gw = gw;
    g_mod->handler     = &tp_handler;
    g_mod->handler_ctx = NULL;
}

void third_party_mgr_start(void) {}
