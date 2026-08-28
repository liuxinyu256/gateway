/**
 * debug_module.c —— 调试模块
 *
 * 用 module_t 框架跑 UART1：
 *   - 周期发送 alive（测试发送链路）
 *   - 收到数据 HEX 回显（测试接收+发送链路）
 */
#include "debug_module.h"
#include <stdio.h>
#include "module.h"
#include "sender.h"
#include "uart_encoder.h"
#include "uart_decoder.h"
#include "uart_instance.h"
#include "receiver_timeout.h"
#include "timer.h"
#include "timer_instance.h"
#ifdef __CH579__
#include "CH57x_common.h"
#endif

typedef struct {
    module_t base;
    uint8_t  rx_buf[128];   /* Debug 模块接收缓冲区 */
    uint8_t  tx_buf[128];   /* HEX 回显格式化缓冲区 */
} debug_module_t;

static debug_module_t     g_dbg;
static sender_t           g_dbg_sender;
static uart_encoder_t     g_dbg_enc;
static uart_decoder_t     g_dbg_dec;
static receiver_timeout_t g_dbg_rx;

static int on_rx_frame(void *ctx, uint8_t *data, uint16_t len)
{
    (void)ctx;

    if (!g_dbg.base.sender)
        return 1;

    /* 命令：S = 查询健康状态 */
    if (len == 1 && (data[0] == 'S' || data[0] == 's')) {
        int n = snprintf((char *)g_dbg.tx_buf, sizeof(g_dbg.tx_buf),
                         "[st] s=%u r=%u st=%u cmd=%u norm=%u\r\n",
                         g_dbg.base.send_queue_drop_cnt,
                         g_dbg.base.receive_queue_drop_cnt,
                         gateway_state_event_drop_count(),
                         frame_queue_drop_count(&g_dbg_sender.cmd_q),
                         frame_queue_drop_count(&g_dbg_sender.norm_q));
        if (n > 0)
            sender_send(g_dbg.base.sender, g_dbg.tx_buf,
                        (uint16_t)n, SENDER_PRIO_CMD);
        return 1;
    }

    int pos = snprintf((char *)g_dbg.tx_buf, sizeof(g_dbg.tx_buf), "[rx]");
    for (uint16_t i = 0; i < len &&
                        pos < (int)sizeof(g_dbg.tx_buf) - 4; i++) {
        pos += snprintf((char *)g_dbg.tx_buf + pos,
                        sizeof(g_dbg.tx_buf) - (size_t)pos,
                        " %02X", data[i]);
    }
    pos += snprintf((char *)g_dbg.tx_buf + pos,
                    sizeof(g_dbg.tx_buf) - (size_t)pos, "\r\n");
    sender_send(g_dbg.base.sender, g_dbg.tx_buf,
                (uint16_t)pos, SENDER_PRIO_CMD);
    return 1;
}

static void on_periodic_send(void *ctx)
{
    (void)ctx;
    static const char alive[] = "alive\r\n";
    if (g_dbg.base.sender)
        sender_send(g_dbg.base.sender, (const uint8_t *)alive,
                    sizeof(alive) - 1, SENDER_PRIO_NORM);
}

static void on_control_cmd(void *ctx, uint8_t cmd, uint8_t val)
{
    (void)ctx; (void)cmd; (void)val;
}

static const event_handler_t debug_evt_table = {
    .on_rx_frame    = on_rx_frame,
    .on_periodic_send = on_periodic_send,
    .on_control_cmd = on_control_cmd,
};

static uint8_t debug_ops_init(module_t *m, void *cfg)
{
    uint32_t baudrate = cfg ? *(uint32_t *)cfg : 115200;
    if (module_base_init(m, baudrate) != 0)
        return 1;
    return 0;
}

static uint8_t *debug_ops_get_rx_buf(module_t *m, uint16_t *size)
{
    debug_module_t *self = (debug_module_t *)m;
    if (!self || !size) return NULL;
    *size = sizeof(self->rx_buf);
    return self->rx_buf;
}

static const module_ops_t debug_module_ops = {
    .init       = debug_ops_init,
    .start      = NULL,
    .get_rx_buf = debug_ops_get_rx_buf,
};

void debug_module_start(void)
{
    uint32_t baudrate = 115200;

#ifdef __CH579__
    SetSysClock(CLK_SOURCE_PLL_32MHz);
    DelayMs(1);
#endif

    g_dbg.base.ops = &debug_module_ops;
    module_set_handler(&g_dbg.base, &debug_evt_table, NULL);

    uart_encoder_cfg_t enc_cfg = {
        .port     = &uart1,
        .uart_cfg = {
            .baudrate  = baudrate,
            .data_bits = 8,
            .stop_bits = 1,
            .parity    = 0,
        },
    };
    uart_encoder_init(&g_dbg_enc, &enc_cfg);

    sender_cfg_t sender_cfg = {
        .encoder = &g_dbg_enc.base,
        .bus     = &g_dbg.base.bus,
    };
    sender_init(&g_dbg_sender, &sender_cfg);
    g_dbg.base.sender = &g_dbg_sender;

    uart_decoder_cfg_t dec_cfg = {
        .port     = &uart1,
        .uart_cfg = {
            .baudrate  = baudrate,
            .data_bits = 8,
            .stop_bits = 1,
            .parity    = 0,
        },
    };
    uart_decoder_init(&g_dbg_dec, &dec_cfg);

    timer_t *rx_timer = timer_hw_create(1);
    receiver_timeout_init(&g_dbg_rx, rx_timer, 5, NULL,
                          g_dbg.rx_buf, sizeof(g_dbg.rx_buf));
    receiver_set_bus(&g_dbg_rx.base, &g_dbg.base.bus);
    uart_decoder_attach_receiver(&g_dbg_dec, &g_dbg_rx.base);
    g_dbg.base.receiver = &g_dbg_rx.base;

    module_init(&g_dbg.base, &baudrate);
    gateway_set_module(1, &g_dbg.base);

    module_start(&g_dbg.base);
}
