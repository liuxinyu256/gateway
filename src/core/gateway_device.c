/**
 * gateway_device.c —— 网关顶层 (每模块独立状态 + 观察者链)
 * 单例模式, 所有接口不传 gw 指针
 */

#include "gateway_device.h"
#include "module.h"
#include <string.h>

static gateway_device_t g_gw;

void gateway_init(void) {
    memset(&g_gw, 0, sizeof(g_gw));
    g_gw.state_mutex = xSemaphoreCreateMutex();
}

/* 模块上报自己的完整状态 */
void gateway_module_state_update(uint8_t module_id,
                                 const gateway_state_t *s)
{
    if (module_id >= GATEWAY_MODULE_MAX || !s)
        return;

    if (g_gw.state_mutex)
        xSemaphoreTake(g_gw.state_mutex, portMAX_DELAY);
    g_gw.module_states[module_id] = *s;
    if (g_gw.state_mutex)
        xSemaphoreGive(g_gw.state_mutex);

    for (uint8_t i = 0; i < g_gw.observer_count; i++) {
        if (g_gw.on_change[i])
            g_gw.on_change[i](module_id, s, g_gw.on_change_ctx[i]);
    }
}

uint8_t gateway_module_state_get(uint8_t module_id,
                                 gateway_state_t *out)
{
    if (module_id >= GATEWAY_MODULE_MAX || !out)
        return 1;

    if (g_gw.state_mutex)
        xSemaphoreTake(g_gw.state_mutex, portMAX_DELAY);
    *out = g_gw.module_states[module_id];
    if (g_gw.state_mutex)
        xSemaphoreGive(g_gw.state_mutex);

    return 0;
}

void gateway_on_state_change(state_change_cb cb, void *ctx) {
    if (g_gw.observer_count >= 8) return;
    g_gw.on_change[g_gw.observer_count]     = cb;
    g_gw.on_change_ctx[g_gw.observer_count] = ctx;
    g_gw.observer_count++;
}

uint8_t gateway_send_cmd(uint8_t module_id, uint8_t cmd, uint8_t val) {
    module_t *m = gateway_module(module_id);
    if (!m) return 1;
    return module_send_cmd(m, cmd, val);
}

module_t *gateway_module(uint8_t id) {
    if (id >= GATEWAY_MODULE_MAX) return NULL;
    return g_gw.modules[id];
}

void gateway_set_module(uint8_t id, module_t *m) {
    if (id < GATEWAY_MODULE_MAX) g_gw.modules[id] = m;
}
