/**
 * ac_phy_rs485.c —— AC 物理层类：UART + RS485 方向控制
 *
 * 485 是独立物理层：基于 UART 编码/解码，外加 DE 方向控制。
 */
#include "ac_phy.h"
#include "hal_io.h"
#include "sender_complete_poll.h"
#include "timer.h"
#include "timer_instance.h"
#include "rs485_ch579.h"
#ifdef __CH579__
#include "CH57x_common.h"
#endif

static uart_encoder_t     s_enc;
static uart_decoder_t     s_dec;
static sender_poll_t      s_sender;
static receiver_timeout_t s_rx;
static rs485_ch579_t      s_rs485;
static uint8_t            s_rx_buf[128];

static uint8_t rs485_create_io(const void *cfg, bus_t *bus, ac_io_t *io)
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

#ifdef __CH579__
    /* RS485 方向控制：DE=PA1（A07S 板） */
    {
        rs485_ch579_cfg_t rs_cfg = {
            .port   = 0,
            .de_pin = GPIO_Pin_1,
        };
        if (rs485_ch579_init(&s_rs485, &rs_cfg) != 0)
            return 1;
    }
    /* 方向控制注入 sender/receiver，bus 本身不感知 485 */
    sender_set_rs485(&s_sender.base, &s_rs485.base);
    receiver_set_rs485(&s_rx.base, &s_rs485.base);
#endif

    io->encoder  = &s_enc.base;
    io->decoder  = &s_dec.base;
    io->sender   = &s_sender.base;
    io->receiver = &s_rx.base;
    io->rs485    = &s_rs485.base;
    return 0;
}

const ac_phy_ops_t ac_phy_rs485_ops = {
    .create_io = rs485_create_io,
};
