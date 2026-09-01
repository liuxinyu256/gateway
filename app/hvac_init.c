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
#include "ac_test.h"
#include "uart_encoder.h"
#include "uart_decoder.h"
#include "uart_instance.h"
#include "sender.h"
#include "sender_complete_poll.h"
#include "receiver_timeout.h"
#include "timer.h"
#include "timer_instance.h"
#include "debug_module.h"
#include "rs485.h"
#include "rs485_ch579.h"
#include "bsp.h"
#ifdef __CH579__
#include "CH57x_common.h"
#endif

#ifdef __CH579__
/* RS485 方向回调适配：bus 层调用 (tx, ctx)，转给 rs485 HAL */
static void hvac_rs485_dir(uint8_t tx, void *ctx)
{
    rs485_set_dir((rs485_t *)ctx, tx);
}
#endif

static ac_module_t        g_ac = { .base.ops = &ac_module_ops };
static rs485_ch579_t      g_hvac_rs485;
static sender_poll_t       g_hvac_sender;
static uart_encoder_t     g_hvac_enc;
static uart_decoder_t     g_hvac_dec;
static receiver_timeout_t g_hvac_rx;
static uint8_t            g_hvac_rx_buf[128];

/* 物理层装配：根据品牌 phy_cfg 创建/配置编码器、解码器、发送器、接收器 */
static uint8_t ac_phy_setup(const ac_phy_cfg_t *phy)
{
    if (!phy)
        return 1;

    switch (phy->phy_type) {
    case AC_PHY_UART: {
        uart_encoder_cfg_t enc_cfg = {
            .port = &uart0,
            .uart_cfg = {
                .baudrate  = phy->baudrate,
                .data_bits = phy->data_bits,
                .stop_bits = phy->stop_bits,
                .parity    = phy->parity,
            },
        };
        uart_encoder_init(&g_hvac_enc, &enc_cfg);

        sender_cfg_t sender_cfg = {
            .encoder = &g_hvac_enc.base,
            .bus     = &g_ac.base.bus,
        };
        sender_poll_init(&g_hvac_sender, &sender_cfg);
        g_ac.base.sender = &g_hvac_sender.base;

        uart_decoder_cfg_t dec_cfg = {
            .port = &uart0,
            .uart_cfg = {
                .baudrate  = phy->baudrate,
                .data_bits = phy->data_bits,
                .stop_bits = phy->stop_bits,
                .parity    = phy->parity,
            },
        };
        uart_decoder_init(&g_hvac_dec, &dec_cfg);

        timer_t *rx_timer = timer_hw_create(0);
        receiver_timeout_init(&g_hvac_rx, rx_timer, phy->receiver_timeout_ticks,
                              NULL, g_hvac_rx_buf, sizeof(g_hvac_rx_buf));
        receiver_set_bus(&g_hvac_rx.base, &g_ac.base.bus);
        uart_decoder_attach_receiver(&g_hvac_dec, &g_hvac_rx.base);
        g_ac.base.receiver = &g_hvac_rx.base;
        return 0;
    }
    default:
        return 1;
    }
}

void hvac_start(void) {
    gateway_init();

    bsp_board_init();   /* AC 模块外围电路选择（按 BSP_BOARD_SELECT 切换） */

    /* RS485, UART0, 9600bps, rx=PB4, tx=PB7, de=PA1 */
#ifdef __CH579__
    /* PA1 作为 RS485 DE，由 rs485 HAL 驱动配置 */
    {
        rs485_ch579_cfg_t rs_cfg = {
            .port   = 0,          /* GPIOA */
            .de_pin = GPIO_Pin_1,
        };
        rs485_ch579_init(&g_hvac_rs485, &rs_cfg);
    }
#endif

    /* 物理层装配：由品牌 phy_cfg 决定编码器/解码器/发送器/接收器 */
    if (ac_phy_setup(ac_test_cfg.phy_cfg) != 0) {
        /* 物理层配置失败，保持不启动 */
        return;
    }

    ac_init_cfg_t cfg = {
        .baudrate    = 9600,
        .brand_table = brand_table,
        .brand_count = AC_BRAND_NUM,
    };

    module_init(&g_ac.base, &cfg);

#ifdef __CH579__
    /* 必须在 module_init 之后设置：module_base_init 会 bus_init 清零 */
    bus_set_rs485_enable(&g_ac.base.bus, 1);
    bus_set_dir_callback(&g_ac.base.bus, hvac_rs485_dir, &g_hvac_rs485.base);
#endif

    /* 注册测试品牌（实现 AC 模块全部事件） */
    ac_module_register(&g_ac, &ac_test_cfg);

    gateway_set_module(0, &g_ac.base);

    module_start(&g_ac.base);

    module_set_poll_period(&g_ac.base, 1000);   /* 测试：1s 周期发读请求 */
    ac_module_start_scan(&g_ac);

    debug_module_start();
}
