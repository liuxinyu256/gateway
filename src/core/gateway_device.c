/**
 * gateway_device.c —— 网关顶层 (每模块独立状态 + 状态事件队列 + 观察者链)
 * 单例模式, 所有接口不传 gw 指针
 */

#include "gateway_device.h"
#include "module.h"
#include <string.h>

#ifdef FAKE_FREERTOS
#define GW_ENTER_CRITICAL()
#define GW_EXIT_CRITICAL()
#else
#define GW_ENTER_CRITICAL() taskENTER_CRITICAL()
#define GW_EXIT_CRITICAL()  taskEXIT_CRITICAL()
#endif

static gateway_device_t g_gw;


static uint8_t gateway_state_enqueue(uint8_t module_id)
{
#ifdef FAKE_FREERTOS
    if (g_gw.state_q_count >= GATEWAY_MODULE_MAX)
        return 1;

    g_gw.state_q_data[g_gw.state_q_tail] = module_id;
    g_gw.state_q_tail = (uint8_t)((g_gw.state_q_tail + 1) % GATEWAY_MODULE_MAX);
    g_gw.state_q_count++;
    return 0;
#else
    return (xQueueSend(g_gw.state_event_queue, &module_id, 0) == pdPASS) ? 0 : 1;
#endif
}

#ifdef FAKE_FREERTOS
static uint8_t gateway_state_dequeue(uint8_t *module_id)
{
    if (g_gw.state_q_count == 0)
        return 1;

    *module_id = g_gw.state_q_data[g_gw.state_q_head];
    g_gw.state_q_head = (uint8_t)((g_gw.state_q_head + 1) % GATEWAY_MODULE_MAX);
    g_gw.state_q_count--;
    return 0;
}
#endif

static void gateway_state_process_event(uint8_t module_id)
{
    gateway_state_t s;

    if (gateway_module_state_get(module_id, &s) != 0)
        return;

    /* 先通知观察者 */
    for (uint8_t i = 0; i < g_gw.observer_count; i++) {
        if (g_gw.on_change[i])
            g_gw.on_change[i](module_id, &s, g_gw.on_change_ctx[i]);
    }

    /* 把完整状态通过 cmd 事件异步同步给其他模块（排除来源模块，避免回声） */
    for (uint8_t i = 0; i < GATEWAY_MODULE_MAX; i++) {
        if (i == module_id)
            continue;
        if (g_gw.modules[i])
            module_send_state_sync(g_gw.modules[i], &s);
    }
}

#ifndef FAKE_FREERTOS
static void gateway_state_task_fn(void *pv)
{
    (void)pv;
    uint8_t module_id;

    for (;;) {
        if (xQueueReceive(g_gw.state_event_queue, &module_id, portMAX_DELAY) == pdPASS) {
            GW_ENTER_CRITICAL();
            g_gw.state_pending[module_id] = 0;
            GW_EXIT_CRITICAL();

            gateway_state_process_event(module_id);
        }
    }
}
#endif

void gateway_init(void) {
    memset(&g_gw, 0, sizeof(g_gw));
    g_gw.state_mutex = xSemaphoreCreateMutex();

#ifndef FAKE_FREERTOS
    g_gw.state_event_queue = xQueueCreate(GATEWAY_MODULE_MAX, sizeof(uint8_t));
    if (g_gw.state_event_queue)
        xTaskCreate(gateway_state_task_fn, "gwstate", 96, NULL, 2,
                    &g_gw.state_task);
#endif
}

/* 模块上报自己的完整状态：先存状态，再投递状态事件（pending 合并） */
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

    GW_ENTER_CRITICAL();

    if (!g_gw.state_pending[module_id]) {
        g_gw.state_pending[module_id] = 1;
        if (gateway_state_enqueue(module_id) != 0) {
            g_gw.state_event_drop_cnt++;
            g_gw.state_pending[module_id] = 0;
        }
    }

    GW_EXIT_CRITICAL();
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

#ifdef FAKE_FREERTOS
void gateway_poll_state_events(void)
{
    uint8_t module_id;

    while (gateway_state_dequeue(&module_id) == 0) {
        g_gw.state_pending[module_id] = 0;
        gateway_state_process_event(module_id);
    }
}
#endif

void gateway_on_state_change(state_change_cb cb, void *ctx) {
    if (g_gw.observer_count >= 8) return;
    g_gw.on_change[g_gw.observer_count]     = cb;
    g_gw.on_change_ctx[g_gw.observer_count] = ctx;
    g_gw.observer_count++;
}

uint16_t gateway_state_event_drop_count(void) {
    return g_gw.state_event_drop_cnt;
}

uint8_t gateway_send_cmd(uint8_t module_id, uint8_t cmd, uint8_t val) {
    module_t *m = gateway_module(module_id);
    if (!m) return 1;
    return module_send_cmd(m, cmd, val);
}

uint8_t gateway_send_event(uint8_t module_id, event_type_t type) {
    module_t *m = gateway_module(module_id);
    if (!m) return 1;
    return module_send_event(m, type);
}

module_t *gateway_module(uint8_t id) {
    if (id >= GATEWAY_MODULE_MAX) return NULL;
    return g_gw.modules[id];
}

void gateway_set_module(uint8_t id, module_t *m) {
    if (id < GATEWAY_MODULE_MAX) g_gw.modules[id] = m;
}
