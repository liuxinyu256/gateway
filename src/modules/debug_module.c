/**
 * debug_module.c —— 调试模块
 *
 * 用 module_t 框架跑 UART1：
 *   - 周期发送 alive（测试发送链路）
 *   - 收到数据 HEX 回显（测试接收+发送链路）
 */
#include "debug_module.h"   /* 含 module.h / stdarg.h */
#include <stdio.h>
#include "FreeRTOS.h"
#include "semphr.h"
#include "uart_encoder.h"
#include "uart_decoder.h"
#include "uart_instance.h"
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

/* 整个用户 RAM 统计（通过链接器 scatter 符号） */
#ifdef __CC_ARM
extern int Image$$RW_IRAM1$$Base;
extern int Image$$RW_IRAM1$$ZI$$Limit;
extern int Image$$RW_IRAM2$$Base;
extern int Image$$RW_IRAM2$$ZI$$Limit;
#endif

static uint32_t debug_app_ram_total(void)
{
    return 0x3000u + 0x2000u;   /* RAM1 12KB + RAM2 8KB */
}

/* 实际占用 = 全局/静态（RW/ZI 减去堆数组） + 堆内已分配 */
static uint32_t debug_app_ram_used(void)
{
#ifdef __CC_ARM
    uint32_t rw_zi =
        ((uint32_t)&Image$$RW_IRAM1$$ZI$$Limit - (uint32_t)&Image$$RW_IRAM1$$Base) +
        ((uint32_t)&Image$$RW_IRAM2$$ZI$$Limit - (uint32_t)&Image$$RW_IRAM2$$Base);
    uint32_t heap_reserved = (uint32_t)configTOTAL_HEAP_SIZE;
    uint32_t static_used = rw_zi > heap_reserved ? rw_zi - heap_reserved : 0;
    uint32_t heap_used = heap_reserved - (uint32_t)xPortGetFreeHeapSize();
    return static_used + heap_used;
#else
    return 0;
#endif
}

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
static SemaphoreHandle_t debug_print_mutex;
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

        uint32_t app_total   = debug_app_ram_total();
        uint32_t app_used    = debug_app_ram_used();
        uint32_t app_free    = app_total - app_used;
        uint32_t app_percent = (app_total > 0) ? (app_used * 100U / app_total) : 0;

        uint32_t heap_free = (uint32_t)xPortGetFreeHeapSize();
        uint32_t heap_min  = (uint32_t)xPortGetMinimumEverFreeHeapSize();

        int n = snprintf((char *)g_dbg.tx_buf, sizeof(g_dbg.tx_buf),
                         "[perf] cpu=%lu.%lu%% ram=%lu%% used=%lu free=%lu total=%lu heap_free=%lu min=%lu\r\n",
                         (unsigned long)(cpu_permille / 10),
                         (unsigned long)(cpu_permille % 10),
                         (unsigned long)app_percent,
                         (unsigned long)app_used, (unsigned long)app_free,
                         (unsigned long)app_total,
                         (unsigned long)heap_free,
                         (unsigned long)heap_min);
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

    /* 命令：A/O/B = 触发 AC 模块事件 */
    if (len == 1 && (data[0] == 'A' || data[0] == 'a')) {
        uint8_t ret = module_send_event(gateway_module(0), EVENT_NEED_ACK);
        int n = snprintf((char *)g_dbg.tx_buf, sizeof(g_dbg.tx_buf),
                         "[evt] need_ack ret=%u\r\n", (unsigned)ret);
        if (n > 0)
            sender_send(g_dbg.base.sender, g_dbg.tx_buf,
                        (uint16_t)n, SENDER_PRIO_CMD);
        return 1;
    }
    if (len == 1 && (data[0] == 'O' || data[0] == 'o')) {
        uint8_t ret = module_send_event(gateway_module(0), EVENT_TIMEOUT);
        int n = snprintf((char *)g_dbg.tx_buf, sizeof(g_dbg.tx_buf),
                         "[evt] timeout ret=%u\r\n", (unsigned)ret);
        if (n > 0)
            sender_send(g_dbg.base.sender, g_dbg.tx_buf,
                        (uint16_t)n, SENDER_PRIO_CMD);
        return 1;
    }
    if (len == 1 && (data[0] == 'B' || data[0] == 'b')) {
        uint8_t ret = module_send_event(gateway_module(0), EVENT_BUS_IDLE);
        int n = snprintf((char *)g_dbg.tx_buf, sizeof(g_dbg.tx_buf),
                         "[evt] bus_idle ret=%u\r\n", (unsigned)ret);
        if (n > 0)
            sender_send(g_dbg.base.sender, g_dbg.tx_buf,
                        (uint16_t)n, SENDER_PRIO_CMD);
        return 1;
    }

    /* 命令：C<cmd>,<val> = 发送控制命令给 AC 模块 */
    if ((data[0] == 'C' || data[0] == 'c') && len >= 5) {
        uint8_t cmd = (uint8_t)(data[1] - '0');
        uint8_t val = (uint8_t)((data[3] - '0') * 10 + (data[4] - '0'));
        uint8_t ret = module_send_cmd(gateway_module(0), cmd, val);
        int n = snprintf((char *)g_dbg.tx_buf, sizeof(g_dbg.tx_buf),
                         "[evt] cmd=%u val=%u ret=%u\r\n",
                         (unsigned)cmd, (unsigned)val, (unsigned)ret);
        if (n > 0)
            sender_send(g_dbg.base.sender, g_dbg.tx_buf,
                        (uint16_t)n, SENDER_PRIO_CMD);
        return 1;
    }

    /* 命令：G = 查询 AC 模块当前状态 */
    if (len == 1 && (data[0] == 'G' || data[0] == 'g')) {
        gateway_state_t s;
        if (gateway_module_state_get(0, &s) == 0) {
            int n = snprintf((char *)g_dbg.tx_buf, sizeof(g_dbg.tx_buf),
                             "[ac st] power=%u mode=%u set=%u room=%u fan=%u swing=%u err=%u\r\n",
                             (unsigned)s.power, (unsigned)s.mode,
                             (unsigned)s.set_temp, (unsigned)s.room_temp,
                             (unsigned)s.fan, (unsigned)s.swing,
                             (unsigned)s.error_code);
            if (n > 0)
                sender_send(g_dbg.base.sender, g_dbg.tx_buf,
                            (uint16_t)n, SENDER_PRIO_CMD);
        } else {
            int n = snprintf((char *)g_dbg.tx_buf, sizeof(g_dbg.tx_buf),
                             "[ac st] unavailable\r\n");
            if (n > 0)
                sender_send(g_dbg.base.sender, g_dbg.tx_buf,
                            (uint16_t)n, SENDER_PRIO_CMD);
        }
        return 1;
    }

    /* 命令：F = 485 物理层测试，直接发送一帧带正确 CRC 的 Modbus 帧 */
    if (len == 1 && (data[0] == 'F' || data[0] == 'f')) {
        static const uint8_t test_frame[] = {
            0x01, 0x03, 0x00, 0x00, 0x00, 0x01, 0x84, 0x0A
        };

        module_t *ac = gateway_module(0);
        if (ac && ac->sender) {
            int dn = snprintf((char *)g_dbg.tx_buf, sizeof(g_dbg.tx_buf),
                              "[dbg] ac bus rs485=%u busy=%u dir=%p gap=%u\r\n",
                              (unsigned)ac->bus.rs485_enable,
                              (unsigned)ac->bus.busy,
                              (void *)ac->bus.set_dir,
                              (unsigned)ac->bus.gap_ms);
            if (dn > 0)
                sender_send(g_dbg.base.sender, g_dbg.tx_buf,
                            (uint16_t)dn, SENDER_PRIO_CMD);

            uint8_t ret = sender_send(ac->sender, test_frame,
                                      sizeof(test_frame), SENDER_PRIO_CMD);
            int n = snprintf((char *)g_dbg.tx_buf, sizeof(g_dbg.tx_buf),
                             "[test] tx:");
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
            0x01, 0x03, 0x00, 0x00, 0x00, 0x01, 0x84, 0x0A
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

static void dbg_frame_done(receiver_t *rx, uint16_t len);
static void dbg_tx_done(sender_t *tx);

static void debug_ops_register_io_callbacks(module_t *m)
{
    if (!m) return;

    if (m->receiver)
        receiver_set_callback(m->receiver, dbg_frame_done);
    if (m->sender) {
        sender_callbacks_t cbs = { .done = dbg_tx_done };
        sender_set_callbacks(m->sender, &cbs);
    }
}

static const module_ops_t debug_module_ops = {
    .init                  = debug_ops_init,
    .start                 = NULL,
    .get_rx_buf            = debug_ops_get_rx_buf,
    .register_io_callbacks = debug_ops_register_io_callbacks,
};

/* Debug 模块自己注册的接收/发送完成回调 */
static void dbg_frame_done(receiver_t *rx, uint16_t len)
{
    (void)rx;
    module_rx_frame_done(&g_dbg.base, len);
}

static void dbg_tx_done(sender_t *tx)
{
    (void)tx;
    module_tx_done(&g_dbg.base);
}

/* 公共发送：复用调试模块 tx_buf，发到 UART1 */
void debug_vprintf(const char *fmt, va_list ap)
{
    int n;

    if (!g_dbg.base.sender)
        return;

    if (debug_print_mutex)
        xSemaphoreTake(debug_print_mutex, portMAX_DELAY);

    n = vsnprintf((char *)g_dbg.tx_buf, sizeof(g_dbg.tx_buf), fmt, ap);
    if (n > 0)
        sender_send(g_dbg.base.sender, g_dbg.tx_buf,
                    (uint16_t)n, SENDER_PRIO_CMD);

    if (debug_print_mutex)
        xSemaphoreGive(debug_print_mutex);
}

void debug_printf(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    debug_vprintf(fmt, ap);
    va_end(ap);
}

void debug_module_start(void)
{
    uint32_t baudrate = 115200;

#ifdef __CH579__
    SetSysClock(CLK_SOURCE_PLL_32MHz);
    DelayMs(1);
#endif

    halLedInit();   /* 运行 LED 初始化 */

    debug_print_mutex = xSemaphoreCreateMutex();

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
