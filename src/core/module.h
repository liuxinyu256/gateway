#ifndef MODULE_H
#define MODULE_H
#include <stdint.h>
#include "receiver.h"
#include "receiver_timeout.h"
#include "sender.h"
#include "bus.h"
#include "uart_decoder.h"
#include "event_handler.h"
#ifdef FAKE_FREERTOS
#include "fake_freertos.h"
#else
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"
#endif

#define RING_HVAC_AC 128
#define RING_THIRD_PARTY 128
#define RING_WIRELESS 64

#define MODULE_MAX 5
#define MODULE_SEND_QUEUE_LEN 8

typedef struct module module_t;

typedef struct module_ops {
    int  (*init)(module_t *m, void *cfg);   /* 模块自己的初始化 */
    void (*start)(module_t *m);             /* 模块自己的启动 (可空) */
} module_ops_t;

typedef struct module
{
    const module_ops_t *ops;     /* 本模块操作表 */
    bus_t bus;
    sender_t sender;
    uart_decoder_t uart_decoder; /* UART 解码器: module_attach_uart() 初始化 */
    receiver_t *rx;              /* 接收器指针 (指向子类提供的接收器实例, 可替换) */
    TaskHandle_t rx_task;        // 接收任务
    TaskHandle_t send_task;      // 发送任务
    QueueHandle_t send_queue;    // 发送队列
    TimerHandle_t poll_timer;    // 轮询软件定时器
    TimerHandle_t timeout_timer; // 超时软件定时器

    const event_handler_t *handler; /* 事件表 (由子类/品牌注册) */
    void                  *handler_ctx;

#ifdef FAKE_FREERTOS
    /* PC 模拟: 用简单环形队列代替 FreeRTOS queue */
    event_t send_q_data[MODULE_SEND_QUEUE_LEN];
    uint8_t send_q_head;
    uint8_t send_q_tail;
    uint8_t send_q_count;
    volatile uint8_t rx_pending; /* 帧完成待处理标志 */
#endif
} module_t;

/* 通用模块接口
 * module_init 只做分发: 调用 m->ops->init(m, cfg)
 * module_base_init 是公共初始化 (bus/模块注册), 由各模块 init 内部调用
 */
int  module_init(module_t *m, void *cfg);
int  module_base_init(module_t *m, uint32_t baudrate);
int  module_attach_uart(module_t *m, uart_t *port, const uart_cfg_t *cfg);
void module_set_handler(module_t *m, const event_handler_t *handler, void *ctx);
void module_start(module_t *m);
int  module_send_cmd(module_t *m, uint8_t cmd, uint8_t val);
void module_set_poll_period(module_t *m, uint16_t period_ms);

#ifdef FAKE_FREERTOS
/* PC 模拟轮询: 处理一次 RX 完成或 send 队列事件 */
int  module_poll(module_t *m);
#endif

#endif
