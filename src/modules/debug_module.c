/**
 * debug_module.c —— 调试模块
 *
 * 用 module_t 框架跑 UART1：
 *   - 周期发送 alive（测试发送链路）
 *   - 收到数据原样回显（测试接收+发送链路）
 */
#include "debug_module.h"
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

static module_t           g_dbg_base;
static sender_t           g_dbg_sender;
static uart_encoder_t     g_dbg_enc;
static uart_decoder_t     g_dbg_dec;
static receiver_timeout_t g_dbg_rx;
static uint8_t            g_dbg_rx_buf[128];

static int on_rx_frame(void *ctx, uint8_t *data, uint16_t len)
{
    (void)ctx;
    if (g_dbg_base.sender)
        sender_send(g_dbg_base.sender, data, len, SENDER_PRIO_CMD);
    return 1;
}

static void on_periodic_send(void *ctx)
{
    (void)ctx;
    static const char alive[] = "alive\r\n";
    if (g_dbg_base.sender)
        sender_send(g_dbg_base.sender, (const uint8_t *)alive,
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

static const module_ops_t debug_module_ops = {
    .init  = debug_ops_init,
    .start = NULL,
};

void debug_module_start(void)
{
    uint32_t baudrate = 115200;

#ifdef __CH579__
    SetSysClock(CLK_SOURCE_PLL_32MHz);
    DelayMs(1);
    /* UART1 默认 PA8(RX)/PA9(TX) */
    GPIOA_ModeCfg(GPIO_Pin_8, GPIO_ModeIN_PU);
    GPIOA_ModeCfg(GPIO_Pin_9, GPIO_ModeOut_PP_5mA);
    GPIOA_SetBits(GPIO_Pin_9);
#endif

    g_dbg_base.ops = &debug_module_ops;
    module_set_handler(&g_dbg_base, &debug_evt_table, NULL);

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
        .bus     = &g_dbg_base.bus,
    };
    sender_init(&g_dbg_sender, &sender_cfg);
    g_dbg_base.sender = &g_dbg_sender;

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
                          g_dbg_rx_buf, sizeof(g_dbg_rx_buf));
    receiver_set_bus(&g_dbg_rx.base, &g_dbg_base.bus);
    uart_decoder_attach_receiver(&g_dbg_dec, &g_dbg_rx.base);
    g_dbg_base.receiver = &g_dbg_rx.base;

    module_init(&g_dbg_base, &baudrate);
    gateway_set_module(1, &g_dbg_base);

    module_start(&g_dbg_base);
}
