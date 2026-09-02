/**
 * ble_module.c —— BLE 模块骨架
 */
#include "ble_module.h"
#include "ble_phy.h"
#include <string.h>

static ble_module_t g_ble = { .base.ops = &ble_module_ops };

static int ble_on_rx(void *ctx, uint8_t *data, uint16_t len)
{
    (void)ctx;
    (void)data;
    (void)len;
    return 1;
}

static void ble_on_gateway_cmd(void *ctx, uint8_t cmd, uint8_t val,
                                const gateway_state_t *state)
{
    ble_module_t *self = (ble_module_t *)ctx;

    if (!self)
        return;

    /* 完整状态同步：BLE 模块镜像网关真相源 */
    if (state) {
        self->base.state = *state;
        return;
    }

    /* 单字段 cmd：按标准命令映射更新 */
    switch (cmd) {
    case 0: self->base.state.power = val; break;
    case 1: self->base.state.mode = val; break;
    case 2: self->base.state.set_temp = val; break;
    case 3: self->base.state.room_temp = val; break;
    case 4: self->base.state.fan = val; break;
    case 5: self->base.state.swing = val; break;
    case 6: self->base.state.error_code = val; break;
    default: break;
    }
}

static const event_handler_t ble_evt_table = {
    .on_activate    = NULL,
    .on_gateway_cmd = ble_on_gateway_cmd,
    .on_rx_frame    = ble_on_rx,
    .on_tick        = NULL,
};

static uint8_t ble_ops_init(module_t *m, void *cfg)
{
    (void)cfg;
    if (!m) return 1;
    return module_base_init(m, 9600); /* BLE 无实际总线，占位波特率 */
}

static uint8_t *ble_ops_get_rx_buf(module_t *m, uint16_t *size)
{
    (void)m;
    if (size) *size = 0;
    return NULL;
}

const module_ops_t ble_module_ops = {
    .init                  = ble_ops_init,
    .start                 = NULL,
    .get_rx_buf            = ble_ops_get_rx_buf,
    .register_io_callbacks = NULL,
};

void ble_module_start(void)
{
    if (ble_phy_init() != 0)
        return;

    g_ble.base.ops = &ble_module_ops;
    module_set_handler(&g_ble.base, &ble_evt_table, &g_ble);

    if (module_init(&g_ble.base, NULL) != 0)
        return;
    gateway_set_module(2, &g_ble.base);

    /* BLE 模块需要 send_task 来接收异步 cmd 状态同步事件 */
    module_start(&g_ble.base);
}
