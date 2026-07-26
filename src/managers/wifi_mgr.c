/**
 * wifi_mgr.c —— WiFi模块管理器
 * UART通信, AT指令协议
 */
#include "wifi_mgr.h"

static module_t        *g_mod;
static gateway_device_t *g_gw;

static void on_periodic(void *ctx) { (void)ctx; /* WiFi保活 */ }
static int  on_rx_frame(void *ctx, uint8_t *d, uint16_t n) {
    (void)ctx;(void)d;(void)n; return 0;
}
static void on_control(void *ctx, uint8_t cmd, uint8_t val) {
    (void)ctx;(void)cmd;(void)val;
}

static const event_handler_t wifi_handler = {
    .on_periodic_send = on_periodic,
    .on_rx_frame      = on_rx_frame,
    .on_control_cmd   = on_control,
};

void wifi_mgr_init(module_t *m, gateway_device_t *gw) {
    g_mod = m; g_gw = gw;
    g_mod->handler     = &wifi_handler;
    g_mod->handler_ctx = NULL;
}

void wifi_mgr_start(void) {}
