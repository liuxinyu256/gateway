/**
 * ble_module.c —— BLE 模块骨架
 *
 * 事件/状态先按标准 module_t 方式占位，
 * 后续把 BLE 回调转成这里的事件。
 */
#include "ble_module.h"
#include "ble_phy.h"
#include "debug_module.h"
#include <string.h>

static ble_module_t g_ble = { .base.ops = &ble_module_ops };

static void ble_on_connect(void *ctx)
{
    ble_module_t *self = (ble_module_t *)ctx;
    gateway_state_t s;

    self->connected = 1;

    if (gateway_module_state_get(2, &s) != 0)
        memset(&s, 0, sizeof(s));
    s.power = 1; /* 示例：连接后置为在线 */
    module_update_state(&self->base, &s);
}

static int ble_on_rx(void *ctx, uint8_t *data, uint16_t len)
{
    (void)ctx;
    (void)data;
    (void)len;
    /* TODO: 解析 APP 下发的数据并更新状态 */
    return 1;
}

static const event_handler_t ble_evt_table = {
    .on_activate     = ble_on_connect,
    .on_control_cmd  = NULL,
    .on_rx_frame     = ble_on_rx,
    .on_tick         = NULL,
};

static uint8_t ble_ops_init(module_t *m, void *cfg)
{
    (void)cfg;
    if (!m) return 1;

    if (module_base_init(m, MODULE_BUS_BLE) != 0)
        return 1;
    return 0;
}

static uint8_t *ble_ops_get_rx_buf(module_t *m, uint16_t *size)
{
    ble_module_t *self = (ble_module_t *)m;
    if (!self || !size) return NULL;
    *size = 0;
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
    /* 先初始化 BLE 物理层（CH57xBLEInit + TMOS 任务），再启动模块 */
    if (ble_phy_init() != 0)
        return;

    g_ble.base.ops = &ble_module_ops;
    module_set_handler(&g_ble.base, &ble_evt_table, &g_ble);

    module_init(&g_ble.base, NULL);
    gateway_set_module(2, &g_ble.base);
    module_start(&g_ble.base);
}
