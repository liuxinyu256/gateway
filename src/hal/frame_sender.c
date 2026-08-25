/**
 * frame_sender.c —— sender_t 的具体实现：帧级队列 + 编码器
 *
 * 生产者：send_task 只往 frame_queue 放完整帧
 * 消费者：sender_pump() 是唯一取帧启动入口
 * 字节搬移：UART THR_EMPTY ISR -> sender_on_thr_empty() -> encoder
 * 帧间 gap：tx_done -> gap timer -> EVENT_BUS_IDLE -> sender_pump()
 */
#include "frame_sender.h"
#include <string.h>

#ifdef FAKE_FREERTOS
#define FS_ENTER_CRITICAL()
#define FS_EXIT_CRITICAL()
#else
#include "FreeRTOS.h"
#include "task.h"
#define FS_ENTER_CRITICAL() taskENTER_CRITICAL()
#define FS_EXIT_CRITICAL()  taskEXIT_CRITICAL()
#endif

extern const sender_ops_t frame_sender_ops;

static uint8_t fs_init(sender_t *s, const void *cfg)
{
    frame_sender_t *tx = (frame_sender_t *)s;
    const frame_sender_cfg_t *c = (const frame_sender_cfg_t *)cfg;

    if (!tx || !c || !c->encoder || !c->bus)
        return 1;

    memset(tx, 0, sizeof(*tx));

    tx->base.ops = &frame_sender_ops;
    tx->encoder  = c->encoder;
    tx->bus      = c->bus;

    frame_queue_init(&tx->cmd_q);
    frame_queue_init(&tx->norm_q);
    return 0;
}

static uint8_t fs_has_pending(const sender_t *s)
{
    const frame_sender_t *tx = (const frame_sender_t *)s;
    if (!tx) return 0;
    return (!frame_queue_empty(&tx->cmd_q) ||
            !frame_queue_empty(&tx->norm_q)) ? 1 : 0;
}

static uint8_t fs_is_wait_tx_complete(const sender_t *s)
{
    const frame_sender_t *tx = (const frame_sender_t *)s;
    return tx ? tx->wait_tx_complete : 0;
}

static uint8_t fs_send_cmd(sender_t *s,
                           const uint8_t *frame, uint16_t len)
{
    frame_sender_t *tx = (frame_sender_t *)s;
    if (!tx || !frame || !len) return 1;

    if (frame_queue_push(&tx->cmd_q, frame, len) != 0)
        return 1;

    sender_pump(s);
    return 0;
}

static uint8_t fs_send(sender_t *s,
                       const uint8_t *frame, uint16_t len)
{
    frame_sender_t *tx = (frame_sender_t *)s;
    if (!tx || !frame || !len) return 1;

    if (frame_queue_push(&tx->norm_q, frame, len) != 0)
        return 1;

    sender_pump(s);
    return 0;
}

static void fs_pump(sender_t *s)
{
    frame_sender_t *tx = (frame_sender_t *)s;
    if (!tx) return;

    FS_ENTER_CRITICAL();

    if (tx->sending) {
        FS_EXIT_CRITICAL();
        return;
    }

    if (!bus_is_idle(tx->bus)) {
        FS_EXIT_CRITICAL();
        return;
    }

    /* CMD 优先 */
    if (frame_queue_pop(&tx->cmd_q, &tx->current) != 0) {
        if (frame_queue_pop(&tx->norm_q, &tx->current) != 0) {
            FS_EXIT_CRITICAL();
            return;
        }
    }

    tx->current_pos      = 0;
    tx->wait_tx_complete = 0;
    tx->sending          = 1;

    bus_mark_busy(tx->bus);
    encoder_tx_enable(tx->encoder);   /* 只开中断，ISR 自己取字节 */

    FS_EXIT_CRITICAL();
}

static void fs_on_thr_empty(sender_t *s)
{
    frame_sender_t *tx = (frame_sender_t *)s;
    if (!tx || !tx->sending || !tx->encoder)
        return;

    if (tx->current_pos < tx->current.len) {
        encoder_encode_byte(tx->encoder,
                            tx->current.data[tx->current_pos++]);
        return;
    }

    /* 当前帧的字节已经全部写进 THR */
    encoder_tx_disable(tx->encoder);

    if (tx->bus && tx->bus->rs485_enable) {
        /* 最后一位还在移位寄存器，不能释放 DE */
        tx->wait_tx_complete = 1;
        if (tx->on_wait_tx_complete)
            tx->on_wait_tx_complete(tx->wait_ctx);
        return;
    }

    tx->sending = 0;
    if (tx->bus)
        bus_on_thr_empty(tx->bus);    /* 非RS485：直接进入 gap */

    if (tx->on_done)
        tx->on_done(tx->done_ctx);    /* tx_done */
}

static void fs_on_tx_complete(sender_t *s)
{
    frame_sender_t *tx = (frame_sender_t *)s;
    if (!tx || !tx->wait_tx_complete)
        return;

    tx->wait_tx_complete = 0;
    tx->sending = 0;

    if (tx->bus)
        bus_on_tx_complete(tx->bus);  /* 释放 DE + 进入 gap */

    if (tx->on_done)
        tx->on_done(tx->done_ctx);    /* tx_done */
}

static void fs_poll_tx_complete(sender_t *s)
{
    frame_sender_t *tx = (frame_sender_t *)s;
    if (!tx || !tx->wait_tx_complete)
        return;

    if (encoder_tx_complete(tx->encoder))
        fs_on_tx_complete(s);
}

static void fs_uart_isr(sender_t *s)
{
    frame_sender_t *tx = (frame_sender_t *)s;
    if (!tx || !tx->sending)
        return;

    if (encoder_tx_ready(tx->encoder))
        fs_on_thr_empty(s);

    if (tx->wait_tx_complete &&
        encoder_tx_complete(tx->encoder))
        fs_on_tx_complete(s);
}

static void fs_set_done_callback(sender_t *s,
                                 void (*cb)(void *ctx), void *ctx)
{
    frame_sender_t *tx = (frame_sender_t *)s;
    if (!tx) return;
    tx->on_done  = cb;
    tx->done_ctx = ctx;
}

static void fs_set_wait_tx_complete_callback(sender_t *s,
                                 void (*cb)(void *ctx), void *ctx)
{
    frame_sender_t *tx = (frame_sender_t *)s;
    if (!tx) return;
    tx->on_wait_tx_complete = cb;
    tx->wait_ctx            = ctx;
}

const sender_ops_t frame_sender_ops = {
    .init                         = fs_init,
    .send_cmd                     = fs_send_cmd,
    .send                         = fs_send,
    .has_pending                  = fs_has_pending,
    .is_wait_tx_complete          = fs_is_wait_tx_complete,
    .pump                         = fs_pump,
    .on_thr_empty                 = fs_on_thr_empty,
    .on_tx_complete               = fs_on_tx_complete,
    .poll_tx_complete             = fs_poll_tx_complete,
    .uart_isr                     = fs_uart_isr,
    .set_done_callback            = fs_set_done_callback,
    .set_wait_tx_complete_callback = fs_set_wait_tx_complete_callback,
};

uint8_t frame_sender_init(frame_sender_t *tx,
                          const frame_sender_cfg_t *cfg)
{
    if (!tx) return 1;
    tx->base.ops = &frame_sender_ops;
    return sender_init(&tx->base, cfg);
}
