/**
 * wireless_module_mgr.c —— 无线模组管理器 (米家WiFi/蓝牙/涂鸦)
 * UART TTL, AT指令/自定义二进制协议
 */
#include "wireless_module_mgr.h"
#include <string.h>

static module_t        *g_mod;
static gateway_device_t *g_gw;

/* ---- 事件处理 (占位, 各模组协议具体实现) ---- */
static void on_periodic(void *ctx) { (void)ctx; /* 心跳保活 */ }
static int  on_rx_frame(void *ctx, uint8_t *d, uint16_t n) {
    (void)ctx;(void)d;(void)n;
    /* 解析模组数据 → 转 gateway_send_cmd 或 gateway_state_update */
    return 0;
}
static void on_control(void *ctx, uint8_t cmd, uint8_t val) {
    (void)ctx;(void)cmd;(void)val;
    /* 来自其他模块的控制 → 发到模组 */
}
static void on_timeout(void *ctx) { (void)ctx; }

static const event_handler_t wireless_handler = {
    .on_periodic_send = on_periodic,
    .on_rx_frame      = on_rx_frame,
    .on_control_cmd   = on_control,
    .on_timeout       = on_timeout,
};

void wireless_module_mgr_init(module_t *m, gateway_device_t *gw) {
    g_mod = m; g_gw = gw;
    g_mod->handler     = &wireless_handler;
    g_mod->handler_ctx = NULL; /* 由各模组协议填充 */
}

void wireless_module_mgr_start(void) {
    /* 初始化后 module_start 会启动任务 */
}
