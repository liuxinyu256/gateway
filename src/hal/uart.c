#include "uart.h"

int uart_configure(uart_t *u, const uart_cfg_t *cfg)
{
    if (!u || !u->ops || !u->ops->configure || !cfg)
        return -1;
    return u->ops->configure(u, cfg);
}

int uart_read(uart_t *u, uint8_t *byte)
{
    if (!u || !u->ops || !u->ops->read || !byte)
        return -1;
    return u->ops->read(u, byte);
}

int uart_write(uart_t *u, uint8_t byte)
{
    if (!u || !u->ops || !u->ops->write)
        return -1;
    return u->ops->write(u, byte);
}

void uart_irq_rx_enable(uart_t *u)
{
    if (!u || !u->ops || !u->ops->irq_rx_enable)
        return;
    u->ops->irq_rx_enable(u);
}

void uart_irq_rx_disable(uart_t *u)
{
    if (!u || !u->ops || !u->ops->irq_rx_disable)
        return;
    u->ops->irq_rx_disable(u);
}

int uart_irq_rx_ready(uart_t *u)
{
    if (!u || !u->ops || !u->ops->irq_rx_ready)
        return 0;
    return u->ops->irq_rx_ready(u);
}

void uart_irq_tx_enable(uart_t *u)
{
    if (!u || !u->ops || !u->ops->irq_tx_enable)
        return;
    u->ops->irq_tx_enable(u);
}

void uart_irq_tx_disable(uart_t *u)
{
    if (!u || !u->ops || !u->ops->irq_tx_disable)
        return;
    u->ops->irq_tx_disable(u);
}

int uart_irq_tx_ready(uart_t *u)
{
    if (!u || !u->ops || !u->ops->irq_tx_ready)
        return 0;
    return u->ops->irq_tx_ready(u);
}

int uart_irq_tx_complete(uart_t *u)
{
    if (!u || !u->ops || !u->ops->irq_tx_complete)
        return 0;
    return u->ops->irq_tx_complete(u);
}

void uart_irq_callback_set(uart_t *u, uart_irq_callback cb, void *user_data)
{
    if (!u) return;

    u->irq_cb        = cb;
    u->irq_user_data = user_data;

    if (u->ops && u->ops->irq_callback_set)
        u->ops->irq_callback_set(u, cb, user_data);
}
