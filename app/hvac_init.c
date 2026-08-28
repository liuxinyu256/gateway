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
#include "debug.h"
#ifdef __CH579__
#include "CH57x_common.h"
#endif

#ifdef __CH579__
/* RS485 DE 方向控制：PA0 高电平=发送，低电平=接收 */
static void hvac_rs485_dir(uint8_t tx, void *ctx)
{
    (void)ctx;
    if (tx)
        GPIOA_SetBits(GPIO_Pin_0);
    else
        GPIOA_ResetBits(GPIO_Pin_0);
}
#endif

static ac_module_t        g_ac = { .base.ops = &ac_module_ops };
static sender_t           g_hvac_sender;
static uart_encoder_t     g_hvac_enc;
static uart_decoder_t     g_hvac_dec;
static receiver_timeout_t g_hvac_rx;
static uint8_t            g_hvac_rx_buf[128];

/* 状态观察者：状态变化时打印（验证状态事件队列） */
static void on_gateway_state_change(uint8_t module_id,
                                    const gateway_state_t *s,
                                    void *ctx)
{
    (void)ctx;
    debug_printf("[state] m=%u set=%u power=%u\r\n",
                 module_id, s->set_temp, s->power);
}

/* 临时 bring-up 心跳：验证调度 + 定期健康快照 */
static void debug_heartbeat_task(void *arg)
{
    (void)arg;
    uint32_t beat = 0;

    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        beat++;
        debug_puts("alive\r\n");

        if ((beat % 5) == 0) {
            gateway_state_t st;
            gateway_module_state_get(g_ac.base.module_id, &st);
            st.set_temp = (uint8_t)(20 + (beat / 5) % 10);
            module_update_state(&g_ac.base, &st);

            debug_printf("[health] s=%u r=%u st=%u cmd=%u norm=%u\r\n",
                         g_ac.base.send_queue_drop_cnt,
                         g_ac.base.receive_queue_drop_cnt,
                         gateway_state_event_drop_count(),
                         frame_queue_drop_count(&g_hvac_sender.cmd_q),
                         frame_queue_drop_count(&g_hvac_sender.norm_q));
        }
    }
}

void hvac_start(void) {
    gateway_init();
    gateway_on_state_change(on_gateway_state_change, NULL);

    xTaskCreate(debug_heartbeat_task, "dbg", 128, NULL, 1, NULL);

    /* RS485, UART0, 9600bps, rx=PB4, tx=PB7, de=PA0 */
#ifdef __CH579__
    /* 绑定 CH579 GPIO：UART0 默认 PB4(RX)/PB7(TX)，PA0 作为 RS485 DE */
    GPIOB_ModeCfg(GPIO_Pin_4, GPIO_ModeIN_PU);
    GPIOB_ModeCfg(GPIO_Pin_7, GPIO_ModeOut_PP_5mA);
    GPIOA_ModeCfg(GPIO_Pin_0, GPIO_ModeOut_PP_5mA);
    GPIOA_ResetBits(GPIO_Pin_0);
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
}
