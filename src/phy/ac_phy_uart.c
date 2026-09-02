/**
 * ac_phy_uart.c —— AC 物理层类：纯 UART
 *
 * 只做 UART 编解码/收发，不包含 RS485 方向控制。
 * 需要 RS485 请使用 ac_phy_rs485。
 */
#include "ac_phy.h"
#include "hal_io.h"
#include "sender_complete_poll.h"
#include "timer.h"
#include "timer_instance.h"

static uart_encoder_t     s_enc;
static uart_decoder_t     s_dec;
static sender_poll_t      s_sender;
static receiver_timeout_t s_rx;
static uint8_t            s_rx_buf[128];

static uint8_t uart_create_io(const void *cfg, bus_t *bus, ac_io_t *io)
{
    const uart_phy_cfg_t *u = (const uart_phy_cfg_t *)cfg;

    if (!u || !bus || !io)
        return 1;

    uart_encoder_cfg_t enc_cfg = {
        .port = &uart0,
        .uart_cfg = {
            .baudrate  = u->baudrate,
            .data_bits = u->data_bits,
            .stop_bits = u->stop_bits,
            .parity    = u->parity,
        },
    };
    if (uart_encoder_init(&s_enc, &enc_cfg) != 0)
        return 1;

    sender_cfg_t sender_cfg = {
        .encoder = &s_enc.base,
        .bus     = bus,
    };
    if (sender_poll_init(&s_sender, &sender_cfg) != 0)
        return 1;

    uart_decoder_cfg_t dec_cfg = {
        .port = &uart0,
        .uart_cfg = {
            .baudrate  = u->baudrate,
            .data_bits = u->data_bits,
            .stop_bits = u->stop_bits,
            .parity    = u->parity,
        },
    };
    if (uart_decoder_init(&s_dec, &dec_cfg) != 0)
        return 1;

    timer_t *rx_timer = timer_hw_create(0);
    receiver_timeout_init(&s_rx, rx_timer, u->receiver_timeout_ticks,
                          NULL, s_rx_buf, sizeof(s_rx_buf));
    receiver_set_bus(&s_rx.base, bus);
    uart_decoder_attach_receiver(&s_dec, &s_rx.base);

    io->encoder  = &s_enc.base;
    io->decoder  = &s_dec.base;
    io->sender   = &s_sender.base;
    io->receiver = &s_rx.base;
    io->rs485    = NULL;
    return 0;
}

const ac_phy_ops_t ac_phy_uart_ops = {
    .create_io = uart_create_io,
};
