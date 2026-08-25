#include "uart_decoder.h"
#include <string.h>

static void uart_decoder_irq_cb(uart_t *u, void *ctx)
{
    (void)u;
    uart_decoder_poll((uart_decoder_t *)ctx);
}

static void decoder_to_receiver(uint8_t byte, void *ctx)
{
    receiver_put_byte((receiver_t *)ctx, byte);
}

static int uart_ops_init(decoder_t *d, const void *cfg)
{
    uart_decoder_t          *u = (uart_decoder_t *)d;
    const uart_decoder_cfg_t *c = (const uart_decoder_cfg_t *)cfg;

    if (!u || !c || !c->port)
        return -1;

    u->port = c->port;

    /* 初始化 UART */
    if (uart_configure(c->port, &c->uart_cfg) != 0)
        return -1;

    /* 解码器注册到 UART：UART 中断回调指向解码器 */
    uart_irq_callback_set(c->port, uart_decoder_irq_cb, u);

    /* 开启接收中断，收到字节后由解码器喂给接收器 */
    uart_irq_rx_enable(c->port);

    return 0;
}

static void uart_ops_feed_byte(decoder_t *d, uint8_t byte)
{
    if (d && d->rx_cb)
        d->rx_cb(byte, d->rx_ctx);
}

static const decoder_ops_t uart_decoder_ops = {
    .init            = uart_ops_init,
    .set_rx_callback = NULL,
    .feed_byte       = uart_ops_feed_byte,
    .feed_sample     = NULL, /* UART 不需要电平采样 */
};

int uart_decoder_init(uart_decoder_t *d, const uart_decoder_cfg_t *cfg)
{
    if (!d || !cfg)
        return -1;

    memset(d, 0, sizeof(*d));
    d->base.ops = &uart_decoder_ops;

    return decoder_init(&d->base, cfg);
}

void uart_decoder_poll(uart_decoder_t *d)
{
    uint8_t byte;

    if (!d || !d->port)
        return;

    while (uart_irq_rx_ready(d->port)) {
        if (uart_read(d->port, &byte) == 0)
            decoder_feed_byte(&d->base, byte);
    }
}

void uart_decoder_attach_receiver(uart_decoder_t *d, receiver_t *rx)
{
    if (!d || !rx)
        return;

    decoder_set_rx_callback(&d->base, decoder_to_receiver, rx);
}
