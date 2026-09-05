#ifndef MODULE_H
#define MODULE_H
#include <stdint.h>
#include "receiver.h"
#include "receiver_timeout.h"
#include "sender.h"
#include "bus.h"
#include "event_handler.h"
#include "gateway_device.h"
#ifdef FAKE_FREERTOS
#include "fake_freertos.h"
#else
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"
#endif

#define MODULE_MAX 5
#define MODULE_EVENT_QUEUE_LEN 4

typedef struct module module_t;

typedef struct module_ops
{
    uint8_t (*init)(module_t *m, void *cfg); /* 模块自己的初始化 */
    void    (*start)(module_t *m);           /* 模块自己的启动 (可空) */
    uint8_t *(*get_rx_buf)(module_t *m, uint16_t *size); /* 返回模块自己的接收缓冲区 */
    void    (*register_io_callbacks)(module_t *m); /* 模块自己注册接收/发送完成回调 */
    void    (*on_event)(module_t *m, const event_t *ev); /* 可选: 模块统一事件日志 */
} module_ops_t;

typedef struct module
{
    const module_ops_t *ops;   /* 本模块操作表 */
    uint8_t    module_id;      /* 注册后的模块编号 (0~MODULE_MAX-1) */

    gateway_state_t state;     /* 模块自己的完整状态 */

    bus_t      bus;            /* 总线状态 */
    sender_t   *sender;        /* 发送抽象：指针注入 */
    receiver_t *receiver;      /* 接收抽象：指针注入 */

    void (*rx_log)(module_t *m, const uint8_t *data, uint16_t len); /* 可选: 运行时由调试模块挂接 */

    TaskHandle_t send_task;    /* 发送任务 */
    TaskHandle_t receive_task; /* 接收任务 */

    QueueHandle_t send_queue;    /* 发送事件队列 */
    QueueHandle_t receive_queue; /* 接收事件队列 */

    volatile uint16_t send_queue_drop_cnt;    /* send_queue 满导致事件丢弃 */
    volatile uint16_t receive_queue_drop_cnt; /* receive_queue 满导致事件丢弃 */

    TimerHandle_t poll_timer;    /* 轮询软件定时器 */
    TimerHandle_t tick_timer;      /* 定时触发软件定时器 */
    TimerHandle_t gap_timer;     /* 帧间 gap 软件定时器 */

    const event_handler_t *handler; /* 事件表 (由子类/品牌注册) */
    void                  *handler_ctx;

#ifdef FAKE_FREERTOS
    /* PC 模拟: 用简单环形队列代替 FreeRTOS queue */
    event_t send_q_data[MODULE_EVENT_QUEUE_LEN];
    uint8_t send_q_head;
    uint8_t send_q_tail;
    uint8_t send_q_count;

    event_t receive_q_data[MODULE_EVENT_QUEUE_LEN];
    uint8_t receive_q_head;
    uint8_t receive_q_tail;
    uint8_t receive_q_count;
#endif
} module_t;

/* 通用模块接口
 * module_init 只做分发: 调用 m->ops->init(m, cfg)
 * module_base_init 是公共初始化 (bus/队列/模块注册), 由各模块 init 内部调用
 */
uint8_t module_init(module_t *m, void *cfg);
uint8_t module_base_init(module_t *m, uint32_t baudrate);
void    module_set_handler(module_t *m, const event_handler_t *handler, void *ctx);
void    module_start(module_t *m);

/* 供各模块自己注册回调时调用的公共辅助 */
void    module_rx_frame_done(module_t *m, uint16_t len); /* 接收完成入队 */
void    module_tx_done(module_t *m);                     /* 发送完成启动 gap (任务上下文) */
void    module_tx_done_from_isr(module_t *m);          /* 发送完成启动 gap (ISR 上下文) */
uint8_t module_send_gateway_cmd(module_t *m, uint8_t cmd, uint8_t val);
uint8_t module_send_state_sync(module_t *m, const gateway_state_t *s); /* 完整状态同步 cmd 事件 */
uint8_t module_send_frame(module_t *m, const uint8_t *data, uint16_t len,
                          uint8_t priority); /* 模块级投帧：由该模块 send_task 发送 */
uint8_t module_send_event(module_t *m, event_type_t type); /* 测试/通用：投递指定事件 */
uint8_t module_send_event_ex(module_t *m, event_type_t type,
                             uint8_t cmd_val, uint8_t cmd_arg); /* 投递带参数事件 */
void    module_set_poll_period(module_t *m, uint16_t period_ms);

/* 模块状态更新/上报：统一走网关状态事件队列 */
void module_update_state(module_t *m, const gateway_state_t *new_state);
void module_publish_state(module_t *m);

#ifdef FAKE_FREERTOS
/* PC 模拟轮询: 处理一次 RX / 发送队列事件 */
uint8_t module_poll(module_t *m);
#endif

#endif
