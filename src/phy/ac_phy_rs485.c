/**
 * ac_phy_rs485.c —— AC 物理层类：UART + RS485 方向控制
 *
 * 485 是独立物理层：基于 UART 编码/解码，外加 DE 方向控制。
 */
#include "ac_phy.h"
#include "bsp.h"
#include "hal_io.h"
#include "sender_complete_poll.h"
#include "receiver_timeout.h"
#include "timer.h"
#include "timer_instance.h"
#include "timer_soft.h"
#include "rs485_ch579.h"
#include <stddef.h>

static uart_encoder_t     s_enc;
static uart_decoder_t     s_dec;
static sender_poll_t      s_sender;
static receiver_timeout_t s_rx;
static rs485_ch579_t      s_rs485;
static uint8_t            s_rx_buf[128];
static uint8_t            s_cmd_ring_buf[256];  /* CMD 帧字节环 */
static uint8_t            s_norm_ring_buf[256]; /* 普通帧字节环 */

/* bus 方向回调适配：bus 层调用 (tx, ctx)，转给 rs485 HAL */
volatile uint32_t g_rs485_dir_tx_cnt;
volatile uint32_t g_rs485_dir_rx_cnt;

uint32_t ac_phy_rs485_tx_dir_count(void) { return g_rs485_dir_tx_cnt; }
uint32_t ac_phy_rs485_rx_dir_count(void) { return g_rs485_dir_rx_cnt; }

static void rs485_bus_dir(uint8_t tx, void *ctx)
{
    if (tx)
        g_rs485_dir_tx_cnt++;
    else
        g_rs485_dir_rx_cnt++;
    rs485_set_dir((rs485_t *)ctx, tx);
}

static uint8_t rs485_create_io(const void *cfg, bus_t *bus, ac_io_t *io)
{
    const uart_phy_cfg_t *u = (const uart_phy_cfg_t *)cfg;

    if (!u || !bus || !io)
        return 1;

    /* 板级引脚/通路选择由 bsp_board_init() 提前完成。
     * 这里只读板级配置拿到 UART 编号与 DE/RE 引脚，不再硬编码 CH579 GPIO。 */
    const bsp_ac_phy_cfg_t *bc = bsp_ac_phy_cfg(bsp_board_get());
    uart_t *port = &uart0;
#ifdef __CH579__
    if (!bc || !bc->de_pin)
        return 1;
    port = uart_get(bc->uart_id);
    if (!port)
        return 1;
#else
    (void)bc;
#endif

    uart_encoder_cfg_t enc_cfg = {
        .port = port,
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
        .encoder        = &s_enc.base,
        .bus            = bus,
        .cmd_ring_buf   = s_cmd_ring_buf,
        .cmd_ring_size  = sizeof(s_cmd_ring_buf),
        .norm_ring_buf  = s_norm_ring_buf,
        .norm_ring_size = sizeof(s_norm_ring_buf),
    };

    if (sender_poll_init(&s_sender, &sender_cfg) != 0)
        return 1;

    /* 共享硬件 tick（1ms）；多个 sender_poll 复用 timer2，只绑定一次 */
    soft_timer_bind_tick(timer_get(2));

    uart_decoder_cfg_t dec_cfg = {
        .port = port,
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
    /* RS485 方向引脚从板级配置读取：换板/换口只改 bsp_a07s.c */
    {
        rs485_ch579_cfg_t rs_cfg = {
            .port   = bc->rs485_port,
            .de_pin = bc->de_pin,
            .re_pin = bc->re_pin,
            .invert = bc->rs485_invert,
        };
        if (rs485_ch579_init(&s_rs485, &rs_cfg) != 0)
            return 1;
    }
    /* 方向控制交给 bus：RS485 只是 bus 方向控制的一种实现 */
    bus_set_dir_callback(bus, rs485_bus_dir, &s_rs485.base);
    bus_set_need_tx_complete(bus, 1);
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
