/**
 * sender.c —— 帧级发送器
 *
 * 生产者：send_task 只往 frame_queue 放完整帧
 * 消费者：sender_pump() 是唯一取帧启动入口
 * 字节搬移：UART ISR / 定时器 tick -> sender_isr() -> encoder
 * 帧间 gap：tx_done -> gap timer -> EVENT_BUS_IDLE -> sender_pump()
 *
 * 发送完成策略（TX_COMPLETE）通过 ops 注入，
 * 具体实现在 sender_complete_poll.c / sender_complete_isr.c。
 */
#include "sender.h"
#include <string.h>

#ifdef FAKE_FREERTOS
#define S_ENTER_CRITICAL()
#define S_EXIT_CRITICAL()
#else
#include "FreeRTOS.h"
#include "task.h"
#define S_ENTER_CRITICAL() taskENTER_CRITICAL()
#define S_EXIT_CRITICAL()  taskEXIT_CRITICAL()
#endif

uint8_t sender_init(sender_t *tx, const sender_cfg_t *cfg)
{
    if (!tx || !cfg || !cfg->encoder || !cfg->bus)
        return 1;

    memset(tx, 0, sizeof(*tx));

    tx->encoder = cfg->encoder;
    tx->bus     = cfg->bus;

    frame_queue_init(&tx->cmd_q);
    frame_queue_init(&tx->norm_q);
    return 0;
}

uint8_t sender_send(sender_t *tx,
                    const uint8_t *frame, uint16_t len,
                    uint8_t priority)
{
    if (!tx || !frame || !len) return 1;

    frame_queue_t *q = priority ? &tx->cmd_q : &tx->norm_q;
    if (frame_queue_push(q, frame, len) != 0)
        return 1;

    sender_pump(tx);
    return 0;
}

void sender_pump(sender_t *tx)
{
    if (!tx) return;

    S_ENTER_CRITICAL();

    if (tx->sending) {
        S_EXIT_CRITICAL();
        return;
    }

    if (!bus_is_idle(tx->bus)) {
        S_EXIT_CRITICAL();
        return;
    }

    /* CMD 优先 */
    if (frame_queue_pop(&tx->cmd_q, &tx->current) != 0) {
        if (frame_queue_pop(&tx->norm_q, &tx->current) != 0) {
            S_EXIT_CRITICAL();
            return;
        }
    }

    tx->current_pos      = 0;
    tx->wait_tx_complete = 0;
    tx->sending          = 1;

    bus_mark_busy(tx->bus);
    encoder_tx_enable(tx->encoder);   /* 只开中断/定时器，ISR/tick 自己取字节 */

    S_EXIT_CRITICAL();
}

static void on_thr_empty(sender_t *tx)
{
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
        if (tx->ops && tx->ops->start)
            tx->ops->start(tx);
        return;
    }

    tx->sending = 0;
    if (tx->bus)
        bus_on_thr_empty(tx->bus);    /* 非RS485：直接进入 gap */

    if (tx->on_done)
        tx->on_done(tx->done_ctx);    /* tx_done */
}

static void on_tx_complete(sender_t *tx)
{
    if (!tx || !tx->wait_tx_complete)
        return;

    tx->wait_tx_complete = 0;
    tx->sending = 0;

    if (tx->ops && tx->ops->stop)
        tx->ops->stop(tx);

    if (tx->bus)
        bus_on_tx_complete(tx->bus);  /* 释放 DE + 进入 gap */

    if (tx->on_done)
        tx->on_done(tx->done_ctx);    /* tx_done */
}

uint8_t sender_poll_tx_complete(sender_t *tx)
{
    if (!tx || !tx->wait_tx_complete)
        return 0;

    if (encoder_tx_complete(tx->encoder)) {
        on_tx_complete(tx);
        return 0;
    }
    return 1;
}

void sender_tx_complete_isr(sender_t *tx)
{
    if (!tx || !tx->wait_tx_complete)
        return;

    if (encoder_tx_complete(tx->encoder))
        on_tx_complete(tx);
}

void sender_isr(sender_t *tx)
{
    if (!tx || !tx->sending)
        return;

    if (encoder_tx_ready(tx->encoder))
        on_thr_empty(tx);

    if (tx->wait_tx_complete &&
        encoder_tx_complete(tx->encoder))
        on_tx_complete(tx);
}

void sender_set_callbacks(sender_t *tx, const sender_callbacks_t *cb)
{
    if (!tx || !cb) return;

    tx->on_done  = cb->done;
    tx->done_ctx = cb->done_ctx;
}
