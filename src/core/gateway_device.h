#ifndef GATEWAY_DEVICE_H
#define GATEWAY_DEVICE_H
#include <stdint.h>
#ifdef FAKE_FREERTOS
#include "fake_freertos.h"
#else
#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"
#include "task.h"
#endif

typedef struct module module_t;

#define GATEWAY_MODULE_MAX 5

/* 统一完整状态
 *
 * 所有模块共用这一个结构：
 *   - 每个模块保存一份自己的完整状态
 *   - 协议只支持其中一部分字段，更新时“读当前完整状态 → 改支持字段 → 写回完整状态”
 *   - 不支持的字段保持不变，从而隔离不同协议差异
 */
typedef struct {
    /* 控制/状态 */
    uint8_t power;
    uint8_t mode;
    uint8_t set_temp;
    uint8_t room_temp;
    uint8_t fan;
    uint8_t swing;
    uint8_t error_code;

    /* 能力范围 (协议上报，不支持的协议不写) */
    uint16_t mode_caps;
    uint16_t fan_caps;
    uint16_t swing_caps;
    uint8_t  temp_min;
    uint8_t  temp_max;
    uint8_t  temp_step;
    uint16_t features;
} gateway_state_t;

typedef void (*state_change_cb)(uint8_t module_id,
                                const gateway_state_t *s,
                                void *ctx);

typedef struct gateway_device {
    gateway_state_t    module_states[GATEWAY_MODULE_MAX];
    SemaphoreHandle_t  state_mutex;

    module_t          *modules[GATEWAY_MODULE_MAX];

    /* 状态变化事件队列：同一模块最多一个 pending 事件 */
    uint8_t            state_pending[GATEWAY_MODULE_MAX];
    volatile uint16_t  state_event_drop_cnt;

#ifdef FAKE_FREERTOS
    uint8_t            state_q_data[GATEWAY_MODULE_MAX];
    uint8_t            state_q_head;
    uint8_t            state_q_tail;
    uint8_t            state_q_count;
#else
    QueueHandle_t      state_event_queue;
    TaskHandle_t       state_task;
#endif

    state_change_cb    on_change[8];
    void              *on_change_ctx[8];
    uint8_t            observer_count;
} gateway_device_t;

void gateway_init(void);
uint8_t gateway_send_cmd(uint8_t module_id, uint8_t cmd, uint8_t val);

/* 模块状态上报/读取：按 module_id 独立保存 */
void    gateway_module_state_update(uint8_t module_id,
                                    const gateway_state_t *s);
uint8_t gateway_module_state_get(uint8_t module_id,
                                 gateway_state_t *out);

void     gateway_on_state_change(state_change_cb cb, void *ctx);
uint16_t gateway_state_event_drop_count(void);
module_t *gateway_module(uint8_t id);
void      gateway_set_module(uint8_t id, module_t *m);

#ifdef FAKE_FREERTOS
/* PC 模拟：手动处理状态事件队列 */
void gateway_poll_state_events(void);
#endif
#endif
