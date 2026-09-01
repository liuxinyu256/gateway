#ifndef SENDER_H
#define SENDER_H
#include <stdint.h>
#include "frame_queue.h"
#include "encoder.h"
#include "bus.h"
#include "sender_complete.h"

/* 发送回调 */
typedef struct
{
    void (*done)(void *ctx); /* 一帧真正发完 (含 RS485 TX_COMPLETE) */
    void *done_ctx;
} sender_callbacks_t;

#define SENDER_PRIO_NORM 0
#define SENDER_PRIO_CMD 1

/* 发送器：帧级队列 + 总线状态机 + 编码器
 *
 * 发送任务组装好完整帧后调用 sender_send()：
 *   - 只入队，不保证立刻发送
 *   - 内部调用 sender_pump()，如果 bus 空闲则取帧启动
 *   - bus 忙时帧不出队，等 EVENT_BUS_IDLE 再 pump
 *
 * 物理层差异通过 encoder_t 注入（UART / 定时器 bit-bang 等）。
 * 发送完成差异通过 complete_ops 注入（轮询 / 中断）。
 */
typedef struct sender
{
    frame_queue_t cmd_q;  /* CMD 帧：优先发 */
    frame_queue_t norm_q; /* 普通帧 */

    encoder_t *encoder; /* 物理层编码器 (UART / 定时器 bit-bang) */
    bus_t *bus;         /* 绑定的发送总线: 与 module 共享同一总线状态机 */

    const sender_complete_ops_t *complete_ops; /* 发送完成策略 */
    void *complete_timer;                      /* 策略私有：poll 用 TimerHandle_t */

    tx_frame_t current; /* 当前正在发送的帧 */
    uint16_t current_pos;

    volatile uint8_t sending;          /* 1 = 当前有帧在发 */
    volatile uint8_t wait_tx_complete; /* RS485: 等最后一位上总线 */

    void (*on_done)(void *ctx);
    void *done_ctx;
} sender_t;

typedef struct
{
    encoder_t *encoder; /* 物理层编码器 */
    bus_t *bus;         /* 要绑定的发送总线 (通常是 module->bus) */
    const sender_complete_ops_t *complete_ops; /* 发送完成策略（可空） */
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

#endif
