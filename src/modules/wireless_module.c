/**
 * wireless_module.c —— 无线模组管理器 (米家WiFi/蓝牙/涂鸦)
 * UART TTL, AT指令/自定义二进制协议
 */
#include "wireless_module.h"

static uint8_t wireless_ops_init(module_t *m, void *cfg)
{
    const wireless_init_cfg_t *c = (const wireless_init_cfg_t *)cfg;
    if (!m || !c) return 1;

    if (module_base_init(m, c->baudrate) != 0)
        return 1;

    wireless_module_init(m, c->gw);
    return 0;
}

const module_ops_t wireless_module_ops = {
    .init  = wireless_ops_init,
    .start = NULL,
};

void wireless_module_init(module_t *m, gateway_device_t *gw)
{
    (void)m;
    (void)gw;
    /* TODO: 无线模组初始化 */
}

void wireless_module_start(void)
{
    /* TODO: 无线模组启动 */
}
