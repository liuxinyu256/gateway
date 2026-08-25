/**
 * wifi_module.c —— WiFi模块管理器
 * UART通信, AT指令协议
 */
#include "wifi_module.h"

static uint8_t wifi_ops_init(module_t *m, void *cfg)
{
    const wifi_init_cfg_t *c = (const wifi_init_cfg_t *)cfg;
    if (!m || !c) return 1;

    if (module_base_init(m, c->baudrate) != 0)
        return 1;

    wifi_module_init(m, c->gw);
    return 0;
}

const module_ops_t wifi_module_ops = {
    .init  = wifi_ops_init,
    .start = NULL,
};

void wifi_module_init(module_t *m, gateway_device_t *gw)
{
    (void)m;
    (void)gw;
    /* TODO: WiFi模块初始化 */
}

void wifi_module_start(void)
{
    /* TODO: WiFi模块启动 */
}
