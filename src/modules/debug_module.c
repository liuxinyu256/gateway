/**
 * debug_module.c —— 调试模块
 *
 * 用 module_t 框架跑 UART1：
 *   - 周期发送 alive（测试发送链路）
 *   - 收到数据 HEX 回显（测试接收+发送链路）
 */
#include "debug_module.h"   /* 含 module.h / stdarg.h */
#include <stdio.h>
#include <string.h>
#include "FreeRTOS.h"
#include "debug_phy.h"
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
} debug_module_t;

static debug_module_t     g_dbg;
static debug_io_t        g_dbg_io;

/* 每个任务独立的静态格式化缓冲区：不占任务栈，也不加锁 */
static char s_log_send_buf[64];   /* AC send_task 文本日志 */
static char s_log_rx_buf[128];    /* AC receive_task 文本/HEX 日志 */
static char s_log_other_buf[64];  /* 其他任务文本日志（当前未用） */
static char s_dbg_rx_buf[128];    /* 调试命令回复使用（Debug receive_task） */

static int cmd_is(const uint8_t *d, uint16_t len, const char *s)
{
    size_t n = strlen(s);
    return len == n && memcmp(d, s, n) == 0;
}

static int on_rx_frame(void *ctx, uint8_t *data, uint16_t len)
{
    char *buf = s_dbg_rx_buf;   /* 本任务专用静态发送缓冲区 */
    (void)ctx;

    if (!g_dbg.base.sender)
        return 1;

    /* 去掉串口助手可能附加的回车/换行/空格 */
    while (len > 0 && (data[len - 1] == '\r' || data[len - 1] == '\n' || data[len - 1] == ' '))
        len--;

    /* 命令：help = 显示命令列表 */
    if (cmd_is(data, len, "help") || (len == 1 && (data[0] == '?' || data[0] == 'h'))) {
        static const char help[] =
            "[cmd] help perf stat tx brand0/1/2 ack timeout idle cmd state\r\n";
        sender_send(g_dbg.base.sender, (const uint8_t *)help,
                    sizeof(help) - 1, SENDER_PRIO_CMD);
        return 1;
    }

    /* 命令：P = CPU + RAM 占用率（CPU 用区间差值 + 一位小数） */
    if (cmd_is(data, len, "perf") || (len == 1 && (data[0] == 'P' || data[0] == 'p'))) {
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

        int n = snprintf((char *)buf, sizeof(s_dbg_rx_buf),
                         "[perf] cpu=%lu.%lu%% ram=%lu%% used=%lu free=%lu total=%lu heap_free=%lu min=%lu\r\n",
                         (unsigned long)(cpu_permille / 10),
                         (unsigned long)(cpu_permille % 10),
                         (unsigned long)app_percent,
                         (unsigned long)app_used, (unsigned long)app_free,
                         (unsigned long)app_total,
                         (unsigned long)heap_free,
                         (unsigned long)heap_min);
        if (n > 0)
            sender_send(g_dbg.base.sender, (const uint8_t *)buf,
                        (uint16_t)n, SENDER_PRIO_CMD);
        return 1;
    }

    /* 命令：S = 查询健康状态 */
    if (cmd_is(data, len, "stat") || (len == 1 && (data[0] == 'S' || data[0] == 's'))) {
        int n = snprintf((char *)buf, sizeof(s_dbg_rx_buf),
                         "[st] s=%u r=%u st=%u cmd=%u norm=%u\r\n",
                         g_dbg.base.send_queue_drop_cnt,
                         g_dbg.base.receive_queue_drop_cnt,
                         gateway_state_event_drop_count(),
                         frame_queue_drop_count(&g_dbg.base.sender->cmd_q),
                         frame_queue_drop_count(&g_dbg.base.sender->norm_q));
        if (n > 0)
            sender_send(g_dbg.base.sender, (const uint8_t *)buf,
                        (uint16_t)n, SENDER_PRIO_CMD);
        return 1;
    }

    /* 命令：A/O/B = 触发 AC 模块事件 */
    if (cmd_is(data, len, "ack") || (len == 1 && (data[0] == 'A' || data[0] == 'a'))) {
        uint8_t ret = module_send_event(gateway_module(0), EVENT_NEED_ACK);
        int n = snprintf((char *)buf, sizeof(s_dbg_rx_buf),
                         "[evt] need_ack ret=%u\r\n", (unsigned)ret);
        if (n > 0)
            sender_send(g_dbg.base.sender, (const uint8_t *)buf,
                        (uint16_t)n, SENDER_PRIO_CMD);
        return 1;
    }
    if (cmd_is(data, len, "timeout") || (len == 1 && (data[0] == 'O' || data[0] == 'o'))) {
        uint8_t ret = module_send_event(gateway_module(0), EVENT_TIMEOUT);
        int n = snprintf((char *)buf, sizeof(s_dbg_rx_buf),
                         "[evt] timeout ret=%u\r\n", (unsigned)ret);
        if (n > 0)
            sender_send(g_dbg.base.sender, (const uint8_t *)buf,
                        (uint16_t)n, SENDER_PRIO_CMD);
        return 1;
    }
    if (cmd_is(data, len, "idle") || (len == 1 && (data[0] == 'B' || data[0] == 'b'))) {
        uint8_t ret = module_send_event(gateway_module(0), EVENT_BUS_IDLE);
        int n = snprintf((char *)buf, sizeof(s_dbg_rx_buf),
                         "[evt] bus_idle ret=%u\r\n", (unsigned)ret);
        if (n > 0)
            sender_send(g_dbg.base.sender, (const uint8_t *)buf,
                        (uint16_t)n, SENDER_PRIO_CMD);
        return 1;
    }

    /* 命令：C<cmd>,<val> 或 cmd<cmd>,<val> = 发送控制命令给 AC 模块 */
    if (((data[0] == 'C' || data[0] == 'c') && len >= 5) ||
        (len >= 7 && memcmp(data, "cmd", 3) == 0)) {
        uint8_t cmd;
        uint8_t val;
        uint8_t ret;

        if (len >= 7 && memcmp(data, "cmd", 3) == 0) {
            /* cmd1,25 */
            cmd = (uint8_t)(data[3] - '0');
            val = (uint8_t)((data[5] - '0') * 10 + (data[6] - '0'));
        } else {
            /* C1,25 */
            cmd = (uint8_t)(data[1] - '0');
            val = (uint8_t)((data[3] - '0') * 10 + (data[4] - '0'));
        }

        ret = module_send_cmd(gateway_module(0), cmd, val);
        int n = snprintf((char *)buf, sizeof(s_dbg_rx_buf),
                         "[evt] cmd=%u val=%u ret=%u\r\n",
                         (unsigned)cmd, (unsigned)val, (unsigned)ret);
        if (n > 0)
            sender_send(g_dbg.base.sender, (const uint8_t *)buf,
                        (uint16_t)n, SENDER_PRIO_CMD);
        return 1;
    }

    /* 命令：G = 查询 AC 模块当前状态 */
    if (cmd_is(data, len, "state") || (len == 1 && (data[0] == 'G' || data[0] == 'g'))) {
        gateway_state_t s;
        if (gateway_module_state_get(0, &s) == 0) {
            int n = snprintf((char *)buf, sizeof(s_dbg_rx_buf),
                             "[ac st] power=%u mode=%u set=%u room=%u fan=%u swing=%u err=%u\r\n",
                             (unsigned)s.power, (unsigned)s.mode,
                             (unsigned)s.set_temp, (unsigned)s.room_temp,
                             (unsigned)s.fan, (unsigned)s.swing,
                             (unsigned)s.error_code);
            if (n > 0)
                sender_send(g_dbg.base.sender, (const uint8_t *)buf,
                            (uint16_t)n, SENDER_PRIO_CMD);
        } else {
            int n = snprintf((char *)buf, sizeof(s_dbg_rx_buf),
                             "[ac st] unavailable\r\n");
            if (n > 0)
                sender_send(g_dbg.base.sender, (const uint8_t *)buf,
                            (uint16_t)n, SENDER_PRIO_CMD);
        }
        return 1;
    }

    /* 命令：F = 485 物理层测试，直接发送一帧带正确 CRC 的 Modbus 帧 */
    if (cmd_is(data, len, "tx") || (len == 1 && (data[0] == 'F' || data[0] == 'f'))) {
        static const uint8_t test_frame[] = {
            0x01, 0x03, 0x00, 0x00, 0x00, 0x01, 0x84, 0x0A
        };

        module_t *ac = gateway_module(0);
        if (ac && ac->sender) {
            int dn = snprintf((char *)buf, sizeof(s_dbg_rx_buf),
                              "[dbg] ac bus busy=%u gap=%u dir=%p need_txc=%u\r\n",
                              (unsigned)ac->bus.busy,
                              (unsigned)ac->bus.gap_ms,
                              (void *)ac->bus.set_dir,
                              (unsigned)ac->bus.need_tx_complete);
            if (dn > 0)
                sender_send(g_dbg.base.sender, (const uint8_t *)buf,
                            (uint16_t)dn, SENDER_PRIO_CMD);

            uint8_t ret = sender_send(ac->sender, test_frame,
                                      sizeof(test_frame), SENDER_PRIO_CMD);
            int n = snprintf((char *)buf, sizeof(s_dbg_rx_buf),
                             "[test] tx:");
            for (uint16_t i = 0; i < sizeof(test_frame) &&
                                n < (int)sizeof(s_dbg_rx_buf) - 8; i++) {
                n += snprintf((char *)buf + n,
                              sizeof(s_dbg_rx_buf) - (size_t)n,
                              " %02X", test_frame[i]);
            }
            n += snprintf((char *)buf + n,
                          sizeof(s_dbg_rx_buf) - (size_t)n,
                          " ret=%u\r\n", (unsigned)ret);
            if (n > 0)
                sender_send(g_dbg.base.sender, (const uint8_t *)buf,
                            (uint16_t)n, SENDER_PRIO_CMD);
        } else {
            int n = snprintf((char *)buf, sizeof(s_dbg_rx_buf),
                             "[test] ac not ready\r\n");
            if (n > 0)
                sender_send(g_dbg.base.sender, (const uint8_t *)buf,
                            (uint16_t)n, SENDER_PRIO_CMD);
        }
        return 1;
    }

    /* 命令：T0/T1/T2、b0/b1/b2、brand0/brand1/brand2 = 切换品牌并发测试帧 */
    {
        int brand_idx = -1;

        if (len == 1 && (data[0] == 'T' || data[0] == 't')) {
            brand_idx = 0;
        } else if (len == 2 && (data[0] == 'T' || data[0] == 't') &&
                   data[1] >= '0' && data[1] <= '2') {
            brand_idx = data[1] - '0';
        } else if (len == 2 && data[0] == 'b' &&
                   data[1] >= '0' && data[1] <= '2') {
            brand_idx = data[1] - '0';
        } else if (cmd_is(data, len, "brand0")) {
            brand_idx = 0;
        } else if (cmd_is(data, len, "brand1")) {
            brand_idx = 1;
        } else if (cmd_is(data, len, "brand2")) {
            brand_idx = 2;
        }

        if (brand_idx >= 0) {
            bsp_ac_brand_t brand = BSP_AC_MEIDI;
            const char *brand_name = "meidi";

            if (brand_idx == 1) { brand = BSP_AC_TOSHIBA; brand_name = "toshiba"; }
            else if (brand_idx == 2) { brand = BSP_AC_HAIER; brand_name = "haier"; }

            bsp_ac_select(bsp_board_get(), brand);

            static const uint8_t test_frame[] = {
                0x01, 0x03, 0x00, 0x00, 0x00, 0x01, 0x84, 0x0A
            };

            module_t *ac = gateway_module(0);
            if (ac && ac->sender) {
                uint8_t ret = sender_send(ac->sender, test_frame,
                                          sizeof(test_frame), SENDER_PRIO_CMD);
                int n = snprintf((char *)buf, sizeof(s_dbg_rx_buf),
                                 "[test] brand=%s tx:", brand_name);
                for (uint16_t i = 0; i < sizeof(test_frame) &&
                                    n < (int)sizeof(s_dbg_rx_buf) - 8; i++) {
                    n += snprintf((char *)buf + n,
                                  sizeof(s_dbg_rx_buf) - (size_t)n,
                                  " %02X", test_frame[i]);
                }
                n += snprintf((char *)buf + n,
                              sizeof(s_dbg_rx_buf) - (size_t)n,
                              " ret=%u\r\n", (unsigned)ret);
                if (n > 0)
                    sender_send(g_dbg.base.sender, (const uint8_t *)buf,
                                (uint16_t)n, SENDER_PRIO_CMD);
            } else {
                int n = snprintf((char *)buf, sizeof(s_dbg_rx_buf),
                                 "[test] ac not ready\r\n");
                if (n > 0)
                    sender_send(g_dbg.base.sender, (const uint8_t *)buf,
                                (uint16_t)n, SENDER_PRIO_CMD);
            }
            return 1;
        }
    }

    int pos = snprintf((char *)buf, sizeof(s_dbg_rx_buf), "[rx]");
    for (uint16_t i = 0; i < len &&
                        pos < (int)sizeof(s_dbg_rx_buf) - 5; i++) {
        pos += snprintf((char *)buf + pos,
                        sizeof(s_dbg_rx_buf) - (size_t)pos,
                        " %02X", data[i]);
    }
    if (pos < (int)sizeof(s_dbg_rx_buf) - 3) {
        pos += snprintf((char *)buf + pos,
                        sizeof(s_dbg_rx_buf) - (size_t)pos, "\r\n");
    } else {
        buf[sizeof(s_dbg_rx_buf) - 1] = 0;
    }
    sender_send(g_dbg.base.sender, (const uint8_t *)buf,
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

/* 根据当前任务选择独立格式化缓冲区：无锁 */
static char *log_get_buffer(uint16_t *size)
{
    module_t *ac = gateway_module(0);
    TaskHandle_t cur = xTaskGetCurrentTaskHandle();

    if (ac && cur == ac->send_task) {
        *size = sizeof(s_log_send_buf);
        return s_log_send_buf;
    }
    if (ac && cur == ac->receive_task) {
        *size = sizeof(s_log_rx_buf);
        return s_log_rx_buf;
    }
    *size = sizeof(s_log_other_buf);
    return s_log_other_buf;
}

/* 公共发送：直接交给 debug 模块自己的 sender，由帧队列异步发送 */
void log_vprintf(const char *fmt, va_list ap)
{
    uint16_t size;
    char *buf = log_get_buffer(&size);
    int n;

    if (!g_dbg.base.sender)
        return;

    n = vsnprintf(buf, size, fmt, ap);
    if (n > 0)
        sender_send(g_dbg.base.sender, (const uint8_t *)buf,
                    (uint16_t)n, SENDER_PRIO_CMD);
}

void log_printf(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    log_vprintf(fmt, ap);
    va_end(ap);
}

/* HEX 打印统一由 debug 模块管理，外部模块只传 tag + 数据 */
void log_hex_dump(const char *tag, const uint8_t *data, uint16_t len)
{
    char *buf = s_log_rx_buf;   /* 当前只有 AC receive_task 调用 */
    int pos;

    if (!g_dbg.base.sender || !data || !len || !tag)
        return;

    pos = snprintf(buf, sizeof(s_log_rx_buf), "[%s] rx:", tag);
    for (uint16_t i = 0; i < len; i++) {
        if (pos + 4 >= (int)sizeof(s_log_rx_buf)) {
            sender_send(g_dbg.base.sender, (const uint8_t *)buf,
                        (uint16_t)pos, SENDER_PRIO_CMD);
            pos = 0;
        }
        pos += snprintf(buf + pos,
                        sizeof(s_log_rx_buf) - (size_t)pos,
                        " %02X", data[i]);
    }
    if (pos + 2 < (int)sizeof(s_log_rx_buf))
        pos += snprintf(buf + pos,
                        sizeof(s_log_rx_buf) - (size_t)pos, "\r\n");
    if (pos > 0)
        sender_send(g_dbg.base.sender, (const uint8_t *)buf,
                    (uint16_t)pos, SENDER_PRIO_CMD);
}

/* 调试模块挂接 AC 模块的 RX 日志：AC 模块自身不感知日志 */
static void dbg_module_rx_log(module_t *m, const uint8_t *data, uint16_t len)
{
    (void)m;
    log_hex_dump("ac evt", data, len);
}

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

    /* 物理层装配：固定 UART1，由 debug_phy_init 创建具体对象并注入 */
    if (debug_phy_init(&g_dbg.base.bus, &g_dbg_io) != 0) {
        return;
    }
    g_dbg.base.sender   = g_dbg_io.sender;
    g_dbg.base.receiver = g_dbg_io.receiver;

    module_init(&g_dbg.base, &baudrate);
    gateway_set_module(1, &g_dbg.base);

    /* RX 日志由 debug 模块统一管理：挂到 AC 模块的运行时日志钩子 */
    {
        module_t *ac = gateway_module(0);
        if (ac)
            ac->rx_log = dbg_module_rx_log;
    }

    module_start(&g_dbg.base);

    /* 心跳改为 30s 一跳 */
    module_set_poll_period(&g_dbg.base, 30000);

}
