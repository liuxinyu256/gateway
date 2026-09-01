/**
 * third_party_module.c —— 第三方485管理器
 * 其他单片机通过RS485和网关通信
 */
#include "third_party_module.h"

static uint8_t third_party_ops_init(module_t *m, void *cfg)
{
    const third_party_init_cfg_t *c = (const third_party_init_cfg_t *)cfg;
    if (!m || !c) return 1;

    if (module_base_init(m, MODULE_BUS_SERIAL) != 0)
        return 1;

    third_party_module_init(m, c->gw);
    return 0;
}

const module_ops_t third_party_module_ops = {
    .init  = third_party_ops_init,
    .start = NULL,
};

void third_party_module_init(module_t *m, gateway_device_t *gw)
{
    (void)m;
    (void)gw;
    /* TODO: 第三方485模块初始化 */
}

void third_party_module_start(void)
{
    /* TODO: 第三方485模块启动 */
}
