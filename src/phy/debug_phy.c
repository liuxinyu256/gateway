/**
 * debug_phy.c —— Debug 模块物理层装配：固定 UART1 115200
 *
 * Debug 也使用重构后的 sender（帧任务 + 双 ring），
 * 只是 ring 给得比 AC 大一些，避免日志突发丢弃。
 */
#include "debug_phy.h"
#include "hal_io.h"
#include "sender_complete_poll.h"
#include "receiver_timeout.h"
#include "timer.h"
#include "timer_instance.h"
#include <stddef.h>

static uart_encoder_t     s_enc;
static uart_decoder_t     s_dec;
static sender_poll_t      s_sender;
static receiver_timeout_t s_rx;
static uint8_t            s_rx_buf[128];   /* 物理层接收环形缓冲区 */

/* Debug 日志/回显主要走 CMD，alive 走 NORM */
static uint8_t s_cmd_ring_buf[1024];
static uint8_t s_norm_ring_buf[512];

uint8_t debug_phy_init(bus_t *bus, debug_io_t *io)
{
    uart_encoder_cfg_t enc_cfg = {
        .port = &uart1,
        .uart_cfg = {
            .baudrate  = 115200,
            .data_bits = 8,
            .stop_bits = 1,
            .parity    = 0,
        },
    };
    if (uart_encoder_init(&s_enc, &enc_cfg) != 0)
        return 1;

    sender_cfg_t sender_cfg = {
        .encoder        = &s_enc.base,
        .bus            = bus,
        .cmd_ring_buf   = s_cmd_ring_buf,
        .cmd_ring_size  = sizeof(s_cmd_ring_buf),
        .norm_ring_buf  = s_norm_ring_buf,
        .norm_ring_size = sizeof(s_norm_ring_buf),
    };
    if (sender_poll_init(&s_sender, &sender_cfg) != 0)
        return 1;

    uart_decoder_cfg_t dec_cfg = {
        .port = &uart1,
        .uart_cfg = {
            .baudrate  = 115200,
            .data_bits = 8,
            .stop_bits = 1,
            .parity    = 0,
        },
    };
    if (uart_decoder_init(&s_dec, &dec_cfg) != 0)
        return 1;

    timer_t *rx_timer = timer_hw_create(1);
    receiver_timeout_init(&s_rx, rx_timer, 5, NULL, s_rx_buf, sizeof(s_rx_buf));
    receiver_set_bus(&s_rx.base, bus);
    uart_decoder_attach_receiver(&s_dec, &s_rx.base);

    io->encoder  = &s_enc.base;
    io->decoder  = &s_dec.base;
    io->sender   = &s_sender.base;
    io->receiver = &s_rx.base;
    return 0;
}
