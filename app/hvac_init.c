/**
 * hvac_init.c —— 网关初始化
 *
 * 上层负责创建并注入：
 *   - receiver_timeout -> m->receiver
 *   - uart_encoder + sender -> m->sender
 * 模块只依赖抽象接口。
 */

#include "hvac_init.h"
#include "gateway.h"
#include "ac_module.h"
#include "uart_encoder.h"
#include "uart_decoder.h"
#include "uart_instance.h"
#include "sender.h"
#include "receiver_timeout.h"
#include "timer.h"
#include "timer_instance.h"
#include "debug_module.h"
#ifdef __CH579__
#include "CH57x_common.h"
#endif

#ifdef __CH579__
/* RS485 DE 方向控制：PA1 高电平=发送，低电平=接收
 * (PA0 已改为运行 LED，不能再做 DE) */
static void hvac_rs485_dir(uint8_t tx, void *ctx)
{
    (void)ctx;
    if (tx)
        GPIOA_SetBits(GPIO_Pin_1);
    else
        GPIOA_ResetBits(GPIO_Pin_1);
}
#endif

static ac_module_t        g_ac = { .base.ops = &ac_module_ops };
static sender_t           g_hvac_sender;
static uart_encoder_t     g_hvac_enc;
static uart_decoder_t     g_hvac_dec;
static receiver_timeout_t g_hvac_rx;
static uint8_t            g_hvac_rx_buf[128];

void hvac_start(void) {
    gateway_init();

    /* RS485, UART0, 9600bps, rx=PB4, tx=PB7, de=PA1 */
#ifdef __CH579__
    /* PA1 作为 RS485 DE，UART0 引脚由 uart_ch579.c 配置 */
    GPIOA_ModeCfg(GPIO_Pin_1, GPIO_ModeOut_PP_5mA);
    GPIOA_ResetBits(GPIO_Pin_1);
    bus_set_rs485_enable(&g_ac.base.bus, 1);
    bus_set_dir_callback(&g_ac.base.bus, hvac_rs485_dir, NULL);
#endif

    /* TX：上层创建 UART 编码器和 sender 并注入 */
    uart_encoder_cfg_t enc_cfg = {
        .port     = &uart0,
        .uart_cfg = {
            .baudrate  = 9600,
            .data_bits = 8,
            .stop_bits = 1,
            .parity    = 0,
        },
    };
    uart_encoder_init(&g_hvac_enc, &enc_cfg);

    sender_cfg_t sender_cfg = {
        .encoder = &g_hvac_enc.base,
        .bus     = &g_ac.base.bus,
    };
    sender_init(&g_hvac_sender, &sender_cfg);
    g_ac.base.sender = &g_hvac_sender;

    /* RX：上层创建 UART 解码器 + 超时接收器并注入
     * 放在 encoder 之后: 最后一次 uart_configure 会开启 RX 中断 */
    uart_decoder_cfg_t dec_cfg = {
        .port     = &uart0,
        .uart_cfg = {
            .baudrate  = 9600,
            .data_bits = 8,
            .stop_bits = 1,
            .parity    = 0,
        },
    };
    uart_decoder_init(&g_hvac_dec, &dec_cfg);

    timer_t *rx_timer = timer_hw_create(0);

    receiver_timeout_init(&g_hvac_rx, rx_timer, 5, NULL,
                          g_hvac_rx_buf, sizeof(g_hvac_rx_buf));
    receiver_set_bus(&g_hvac_rx.base, &g_ac.base.bus);
    uart_decoder_attach_receiver(&g_hvac_dec, &g_hvac_rx.base);
    g_ac.base.receiver = &g_hvac_rx.base;

    ac_init_cfg_t cfg = {
        .baudrate    = 9600,
        .brand_table = brand_table,
        .brand_count = AC_BRAND_NUM,
    };

    module_init(&g_ac.base, &cfg);
    gateway_set_module(0, &g_ac.base);

    module_start(&g_ac.base);
    ac_module_start_scan(&g_ac);

    debug_module_start();
}
