#include "uart_encoder.h"
#include <string.h>

static uint8_t uart_encoder_configure(encoder_t *e, const void *cfg)
{
    const uart_encoder_cfg_t *c = (const uart_encoder_cfg_t *)cfg;
    uart_encoder_t           *u = (uart_encoder_t *)e;

    if (!u || !c || !c->port)
        return 1;

    u->port = c->port;
    return (uart_configure(c->port, &c->uart_cfg) == 0) ? 0 : 1;
}

static uint8_t uart_encoder_encode_byte(encoder_t *e, uint8_t byte)
{
    uart_encoder_t *u = (uart_encoder_t *)e;
    return (uart_write(u->port, byte) == 0) ? 0 : 1;
}

static void uart_encoder_tx_enable(encoder_t *e)
{
    uart_encoder_t *u = (uart_encoder_t *)e;
    uart_irq_tx_enable(u->port);
}

static void uart_encoder_tx_disable(encoder_t *e)
{
    uart_encoder_t *u = (uart_encoder_t *)e;
    uart_irq_tx_disable(u->port);
}

static uint8_t uart_encoder_tx_ready(encoder_t *e)
{
    uart_encoder_t *u = (uart_encoder_t *)e;
    return uart_irq_tx_ready(u->port) ? 1 : 0;
}

static uint8_t uart_encoder_tx_complete(encoder_t *e)
{
    uart_encoder_t *u = (uart_encoder_t *)e;
    return uart_irq_tx_complete(u->port) ? 1 : 0;
}

static const encoder_ops_t uart_encoder_ops = {
    .configure    = uart_encoder_configure,
    .encode_byte  = uart_encoder_encode_byte,
    .tx_enable    = uart_encoder_tx_enable,
    .tx_disable   = uart_encoder_tx_disable,
    .tx_ready     = uart_encoder_tx_ready,
    .tx_complete  = uart_encoder_tx_complete,
};

uint8_t uart_encoder_init(uart_encoder_t *e,
                          const uart_encoder_cfg_t *cfg)
{
    if (!e || !cfg || !cfg->port)
        return 1;

    memset(e, 0, sizeof(*e));

    e->base.ops = &uart_encoder_ops;

    return encoder_configure(&e->base, cfg);
}
