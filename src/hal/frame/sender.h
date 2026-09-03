#ifndef SENDER_H
#define SENDER_H
#include <stdint.h>
#include "ring.h"
#include "encoder.h"
#include "bus.h"
#include "sender_complete.h"

/* 发送完成回调类型（与接收 frame_finish_callback 对称） */
typedef void (*sender_done_callback)(sender_t *tx);

/* 发送回调 */
typedef struct
{
    sender_done_callback done; /* 一帧真正发完 (含 RS485 TX_COMPLETE) */
} sender_callbacks_t;

#define SENDER_PRIO_NORM 0
#define SENDER_PRIO_CMD 1

/* 帧任务队列：不存整帧，只记录帧长度和发送进度 */
#define TX_JOB_QUEUE_LEN   8

typedef struct {
    uint16_t len;
    uint16_t sent;
} tx_job_t;

typedef struct {
    tx_job_t jobs[TX_JOB_QUEUE_LEN];
    uint8_t head;
    uint8_t tail;
    uint8_t count;
} tx_job_queue_t;

/* 发送器：帧级队列 + 字节环形缓冲 + 总线状态机 + 编码器
 *
 * 环形缓冲的存储数组由调用方/模块外部提供，大小可各自指定。
 */
typedef struct sender
{
    const sender_ops_t *ops; /* 发送完成策略（由子类设置），首成员与 receiver 对齐 */

    tx_job_queue_t cmd_jobs;   /* CMD 帧任务 */
    tx_job_queue_t norm_jobs;  /* 普通帧任务 */

    ring_t cmd_ring;           /* CMD 帧字节环（管理信息） */
    ring_t norm_ring;          /* 普通帧字节环（管理信息） */

    uint8_t current_prio;      /* 当前正在发：0=norm, 1=cmd */
    tx_job_t current;          /* 当前帧任务 */

    encoder_t *encoder;        /* 物理层编码器 (UART / 定时器 bit-bang) */
    bus_t *bus;                /* 绑定的发送总线 */

    volatile uint8_t sending;          /* 1 = 当前有帧在发 */
    volatile uint8_t wait_tx_complete; /* 方向控制需等最后一位上总线 */
    volatile uint16_t drop_cnt;        /* ring/任务队列满导致丢帧计数 */

    sender_done_callback on_done;
} sender_t;

typedef struct
{
    encoder_t *encoder; /* 物理层编码器 */
    bus_t     *bus;     /* 要绑定的发送总线 (通常是 module->bus) */

    /* 环形缓冲存储由外部提供，每个模块可以给不同大小 */
    uint8_t  *cmd_ring_buf;
    uint16_t  cmd_ring_size;
    uint8_t  *norm_ring_buf;
    uint16_t  norm_ring_size;
} sender_cfg_t;

uint8_t sender_init(sender_t *tx, const sender_cfg_t *cfg);
uint8_t sender_send(sender_t *tx,
                    const uint8_t *frame, uint16_t len,
                    uint8_t priority);
void sender_pump(sender_t *tx);
void sender_isr(sender_t *tx);                 /* UART ISR 或定时器 tick */
uint8_t sender_poll_tx_complete(sender_t *tx); /* 1=仍在等 TX_COMPLETE */
void sender_tx_complete_isr(sender_t *tx);     /* ISR 策略：外部中断调用 */
void sender_set_callbacks(sender_t *tx, const sender_callbacks_t *cb);
uint16_t sender_drop_count(const sender_t *tx);

#endif
