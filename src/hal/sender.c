/**
 * sender.c —— 发送器基类分发，与 receiver.c / decoder.c 同一风格。
 */
#include "sender.h"

uint8_t sender_init(sender_t *tx, const void *cfg)
{
    if (!tx || !tx->ops || !tx->ops->init) return 1;
    return tx->ops->init(tx, cfg);
}

uint8_t sender_send_cmd(sender_t *tx,
                        const uint8_t *frame, uint16_t len)
{
    if (!tx || !tx->ops || !tx->ops->send_cmd) return 1;
    return tx->ops->send_cmd(tx, frame, len);
}

uint8_t sender_send(sender_t *tx,
                    const uint8_t *frame, uint16_t len)
{
    if (!tx || !tx->ops || !tx->ops->send) return 1;
    return tx->ops->send(tx, frame, len);
}

uint8_t sender_has_pending(const sender_t *tx)
{
    if (!tx || !tx->ops || !tx->ops->has_pending) return 0;
    return tx->ops->has_pending(tx);
}

uint8_t sender_is_wait_tx_complete(const sender_t *tx)
{
    if (!tx || !tx->ops || !tx->ops->is_wait_tx_complete) return 0;
    return tx->ops->is_wait_tx_complete(tx);
}

void sender_pump(sender_t *tx)
{
    if (tx && tx->ops && tx->ops->pump)
        tx->ops->pump(tx);
}

void sender_on_thr_empty(sender_t *tx)
{
    if (tx && tx->ops && tx->ops->on_thr_empty)
        tx->ops->on_thr_empty(tx);
}

void sender_on_tx_complete(sender_t *tx)
{
    if (tx && tx->ops && tx->ops->on_tx_complete)
        tx->ops->on_tx_complete(tx);
}

void sender_poll_tx_complete(sender_t *tx)
{
    if (tx && tx->ops && tx->ops->poll_tx_complete)
        tx->ops->poll_tx_complete(tx);
}

void sender_uart_isr(sender_t *tx)
{
    if (tx && tx->ops && tx->ops->uart_isr)
        tx->ops->uart_isr(tx);
}

void sender_set_done_callback(sender_t *tx,
                              void (*cb)(void *ctx), void *ctx)
{
    if (tx && tx->ops && tx->ops->set_done_callback)
        tx->ops->set_done_callback(tx, cb, ctx);
}

void sender_set_wait_tx_complete_callback(sender_t *tx,
                              void (*cb)(void *ctx), void *ctx)
{
    if (tx && tx->ops && tx->ops->set_wait_tx_complete_callback)
        tx->ops->set_wait_tx_complete_callback(tx, cb, ctx);
}
