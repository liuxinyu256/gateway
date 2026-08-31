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
#include "led.h"
#include "bsp.h"
#include "gateway.h"
#ifdef __CH579__
#include "CH57x_common.h"
#endif

/* 空闲钩子：统计空闲 tick，用于计算 CPU 占用率 */
static volatile uint32_t debug_idle_ticks;
static TickType_t debug_last_idle_tick;

void vApplicationIdleHook(void)
{
    TickType_t now = xTaskGetTickCount();
    if (now != debug_last_idle_tick) {
        debug_last_idle_tick = now;
        debug_idle_ticks++;
    }
}

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

    /* 命令：P = CPU + RAM 占用率（CPU 用区间差值 + 一位小数） */
    if (len == 1 && (data[0] == 'P' || data[0] == 'p')) {
        static uint32_t last_total_tick;
        static uint32_t last_idle_tick;

        uint32_t total_now = xTaskGetTickCount();
        uint32_t idle_now  = debug_idle_ticks;
        uint32_t d_total = total_now - last_total_tick;
        uint32_t d_idle  = idle_now - last_idle_tick;
        uint32_t d_busy  = d_total - (d_idle < d_total ? d_idle : d_total);
        uint32_t cpu_permille = (d_total > 0) ? (d_busy * 1000U / d_total) : 0;
        last_total_tick = total_now;
        last_idle_tick  = idle_now;

        uint32_t ram_total = (uint32_t)configTOTAL_HEAP_SIZE;
        uint32_t ram_free  = (uint32_t)xPortGetFreeHeapSize();
        uint32_t ram_used  = ram_total - ram_free;
        uint32_t ram_min   = (uint32_t)xPortGetMinimumEverFreeHeapSize();
        uint32_t ram_percent = (ram_total > 0) ? (ram_used * 100U / ram_total) : 0;

        int n = snprintf((char *)g_dbg.tx_buf, sizeof(g_dbg.tx_buf),
                         "[perf] cpu=%lu.%lu%% ram=%lu%% used=%lu free=%lu total=%lu min=%lu\r\n",
                         (unsigned long)(cpu_permille / 10),
                         (unsigned long)(cpu_permille % 10),
                         (unsigned long)ram_percent,
                         (unsigned long)ram_used, (unsigned long)ram_free,
                         (unsigned long)ram_total, (unsigned long)ram_min);
        if (n > 0)
            sender_send(g_dbg.base.sender, g_dbg.tx_buf,
                        (uint16_t)n, SENDER_PRIO_CMD);
        return 1;
    }

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

    /* 命令：T[0|1|2] = 485 测试，切换品牌并发一帧测试数据
     *   T0/美的  T1/东芝  T2/海尔
     */
    if ((data[0] == 'T' || data[0] == 't') && (len == 1 || len == 2)) {
        bsp_ac_brand_t brand = BSP_AC_MEIDI;
        const char *brand_name = "meidi";

        if (len == 2) {
            if (data[1] == '1') { brand = BSP_AC_TOSHIBA; brand_name = "toshiba"; }
            else if (data[1] == '2') { brand = BSP_AC_HAIER; brand_name = "haier"; }
        }

        bsp_ac_select(bsp_board_get(), brand);

        static const uint8_t test_frame[] = {
            0x01, 0x03, 0x00, 0x00, 0x00, 0x01, 0x0A, 0x0B
        };

        module_t *ac = gateway_module(0);
        if (ac && ac->sender) {
            uint8_t ret = sender_send(ac->sender, test_frame,
                                      sizeof(test_frame), SENDER_PRIO_CMD);
            int n = snprintf((char *)g_dbg.tx_buf, sizeof(g_dbg.tx_buf),
                             "[test] brand=%s tx:", brand_name);
            for (uint16_t i = 0; i < sizeof(test_frame) &&
                                n < (int)sizeof(g_dbg.tx_buf) - 8; i++) {
                n += snprintf((char *)g_dbg.tx_buf + n,
                              sizeof(g_dbg.tx_buf) - (size_t)n,
                              " %02X", test_frame[i]);
            }
            n += snprintf((char *)g_dbg.tx_buf + n,
                          sizeof(g_dbg.tx_buf) - (size_t)n,
                          " ret=%u\r\n", (unsigned)ret);
            if (n > 0)
                sender_send(g_dbg.base.sender, g_dbg.tx_buf,
                            (uint16_t)n, SENDER_PRIO_CMD);
        } else {
            int n = snprintf((char *)g_dbg.tx_buf, sizeof(g_dbg.tx_buf),
                             "[test] ac not ready\r\n");
            if (n > 0)
                sender_send(g_dbg.base.sender, g_dbg.tx_buf,
                            (uint16_t)n, SENDER_PRIO_CMD);
        }
        return 1;
    }

    int pos = snprintf((char *)g_dbg.tx_buf, sizeof(g_dbg.tx_buf), "[rx]");
    for (uint16_t i = 0; i < len &&
                        pos < (int)sizeof(g_dbg.tx_buf) - 5; i++) {
        pos += snprintf((char *)g_dbg.tx_buf + pos,
                        sizeof(g_dbg.tx_buf) - (size_t)pos,
                        " %02X", data[i]);
    }
    if (pos < (int)sizeof(g_dbg.tx_buf) - 3) {
        pos += snprintf((char *)g_dbg.tx_buf + pos,
                        sizeof(g_dbg.tx_buf) - (size_t)pos, "\r\n");
    } else {
        g_dbg.tx_buf[sizeof(g_dbg.tx_buf) - 1] = 0;
    }
    sender_send(g_dbg.base.sender, g_dbg.tx_buf,
                (uint16_t)pos, SENDER_PRIO_CMD);
    return 1;
}

static void on_periodic_send(void *ctx)
{
    (void)ctx;
    static const char alive[] = "alive\r\n";
    halLedRunBlink();   /* 每次 alive 翻转一次运行 LED */
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

    halLedInit();   /* 运行 LED 初始化 */

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
