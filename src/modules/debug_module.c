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
#include "semphr.h"
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
    return 0x3000u + 0x2800u;   /* RAM1 12KB + RAM2 10KB（实际链接 obj/gateway.sct） */
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

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask;
    (void)pcTaskName;
    /* 栈溢出：软件陷阱，便于 J-Link halt 查看任务名 */
    __disable_irq();
    for (;;) {
    }
}

typedef struct {
    module_t base;
    uint8_t  rx_buf[128];   /* Debug 模块接收缓冲区 */
} debug_module_t;

static debug_module_t     g_dbg;
static debug_io_t        g_dbg_io;
static SemaphoreHandle_t log_print_mutex;

/* 每个任务独立的静态格式化缓冲区：不占任务栈，也不加锁 */
static char s_log_send_buf[64];   /* AC send_task 文本日志 */
static char s_log_rx_buf[128];    /* AC receive_task 文本/HEX 日志 */
static char s_log_other_buf[64];  /* 其他任务文本日志（当前未用） */
static char s_dbg_rx_buf[128];    /* 调试命令回复使用（Debug receive_task） */

/* 日志开关：心跳默认开，其他默认关 */
static uint8_t s_log_heartbeat_enabled = 1;
static uint8_t s_log_event_enabled     = 0;
static uint8_t s_log_rx_enabled        = 0;

/* ---- Debug TX 通过 send_queue 投递给 Debug send_task 发送 ---- */
#define DEBUG_TX_SLOTS   4
#define DEBUG_TX_MSG_MAX 128

typedef struct {
    uint8_t  data[DEBUG_TX_MSG_MAX];
    uint16_t len;
    uint8_t  prio;
} debug_tx_msg_t;

static debug_tx_msg_t g_dbg_tx_slots[DEBUG_TX_SLOTS];
static uint8_t        g_dbg_tx_slot_used[DEBUG_TX_SLOTS];
static QueueHandle_t  g_dbg_tx_free_queue;

static uint8_t debug_tx_enqueue_ex(const uint8_t *data, uint16_t len, uint8_t prio);
static void    debug_ops_on_event(module_t *m, const event_t *ev);
static uint8_t ac_send_test_frame(const uint8_t *data, uint16_t len);

uint8_t log_event_enabled(void) { return s_log_event_enabled; }
uint8_t log_rx_enabled(void)    { return s_log_rx_enabled; }

static int cmd_is(const uint8_t *d, uint16_t len, const char *s)
{
    size_t n = strlen(s);
    return len == n && memcmp(d, s, n) == 0;
}

static int debug_cmd_perf(uint8_t *data, uint16_t len)
{
    char *buf = s_dbg_rx_buf;
    static uint32_t last_total_tick;
    static uint32_t last_idle_tick;

    if (!cmd_is(data, len, "perf") && !(len == 1 && (data[0] == 'P' || data[0] == 'p')))
        return 0;

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
        debug_tx_enqueue_ex((const uint8_t *)buf, (uint16_t)n, SENDER_PRIO_CMD);
    return 1;
}

static int debug_cmd_stat(uint8_t *data, uint16_t len)
{
    char *buf = s_dbg_rx_buf;
    int n;

    if (!cmd_is(data, len, "stat") && !(len == 1 && (data[0] == 'S' || data[0] == 's')))
        return 0;

    n = snprintf((char *)buf, sizeof(s_dbg_rx_buf),
                 "[st] s=%u r=%u st=%u tx=%u rx=%u\r\n",
                 g_dbg.base.send_queue_drop_cnt,
                 g_dbg.base.receive_queue_drop_cnt,
                 gateway_state_event_drop_count(),
                 sender_drop_count(g_dbg.base.sender),
                 receiver_frame_drop_count(g_dbg.base.receiver));
    if (n > 0)
        debug_tx_enqueue_ex((const uint8_t *)buf, (uint16_t)n, SENDER_PRIO_CMD);
    return 1;
}

static int debug_cmd_ack(uint8_t *data, uint16_t len)
{
    char *buf = s_dbg_rx_buf;
    uint8_t ret;
    int n;

    if (!cmd_is(data, len, "ack") && !(len == 1 && (data[0] == 'A' || data[0] == 'a')))
        return 0;

    ret = gateway_send_event(0, EVENT_NEED_ACK);
    n = snprintf((char *)buf, sizeof(s_dbg_rx_buf),
                 "[evt] need_ack ret=%u\r\n", (unsigned)ret);
    if (n > 0)
        debug_tx_enqueue_ex((const uint8_t *)buf, (uint16_t)n, SENDER_PRIO_CMD);
    return 1;
}

static int debug_cmd_tick(uint8_t *data, uint16_t len)
{
    char *buf = s_dbg_rx_buf;
    uint8_t ret;
    int n;

    if (!cmd_is(data, len, "tick") && !cmd_is(data, len, "timeout") &&
        !(len == 1 && (data[0] == 'O' || data[0] == 'o')))
        return 0;

    ret = gateway_send_event(0, EVENT_TICK);
    n = snprintf((char *)buf, sizeof(s_dbg_rx_buf),
                 "[evt] tick ret=%u\r\n", (unsigned)ret);
    if (n > 0)
        debug_tx_enqueue_ex((const uint8_t *)buf, (uint16_t)n, SENDER_PRIO_CMD);
    return 1;
}

static int debug_cmd_idle(uint8_t *data, uint16_t len)
{
    char *buf = s_dbg_rx_buf;
    uint8_t ret;
    int n;

    if (!cmd_is(data, len, "idle") && !(len == 1 && (data[0] == 'B' || data[0] == 'b')))
        return 0;

    ret = gateway_send_event(0, EVENT_BUS_IDLE);
    n = snprintf((char *)buf, sizeof(s_dbg_rx_buf),
                 "[evt] bus_idle ret=%u\r\n", (unsigned)ret);
    if (n > 0)
        debug_tx_enqueue_ex((const uint8_t *)buf, (uint16_t)n, SENDER_PRIO_CMD);
    return 1;
}

static int debug_cmd_ctrl(uint8_t *data, uint16_t len)
{
    char *buf = s_dbg_rx_buf;
    uint8_t cmd;
    uint8_t val;
    uint8_t ret;
    int n;

    if (!((data[0] == 'C' || data[0] == 'c') && len >= 5) &&
        !(len >= 7 && memcmp(data, "cmd", 3) == 0))
        return 0;

    if (len >= 7 && memcmp(data, "cmd", 3) == 0) {
        /* cmd1,25 */
        cmd = (uint8_t)(data[3] - '0');
        val = (uint8_t)((data[5] - '0') * 10 + (data[6] - '0'));
    } else {
        /* C1,25 */
        cmd = (uint8_t)(data[1] - '0');
        val = (uint8_t)((data[3] - '0') * 10 + (data[4] - '0'));
    }

    ret = gateway_send_cmd(0, cmd, val);
    n = snprintf((char *)buf, sizeof(s_dbg_rx_buf),
                 "[evt] cmd=%u val=%u ret=%u\r\n",
                 (unsigned)cmd, (unsigned)val, (unsigned)ret);
    if (n > 0)
        debug_tx_enqueue_ex((const uint8_t *)buf, (uint16_t)n, SENDER_PRIO_CMD);
    return 1;
}

static int debug_cmd_state(uint8_t *data, uint16_t len)
{
    char *buf = s_dbg_rx_buf;
    gateway_state_t s;
    int n;

    if (!cmd_is(data, len, "state") && !(len == 1 && (data[0] == 'G' || data[0] == 'g')))
        return 0;

    if (gateway_module_state_get(0, &s) == 0) {
        n = snprintf((char *)buf, sizeof(s_dbg_rx_buf),
                     "[ac st] power=%u mode=%u set=%u room=%u fan=%u swing=%u err=%u\r\n",
                     (unsigned)s.power, (unsigned)s.mode,
                     (unsigned)s.set_temp, (unsigned)s.room_temp,
                     (unsigned)s.fan, (unsigned)s.swing,
                     (unsigned)s.error_code);
    } else {
        n = snprintf((char *)buf, sizeof(s_dbg_rx_buf),
                     "[ac st] unavailable\r\n");
    }
    if (n > 0)
        debug_tx_enqueue_ex((const uint8_t *)buf, (uint16_t)n, SENDER_PRIO_CMD);
    return 1;
}

static int debug_cmd_tx(uint8_t *data, uint16_t len)
{
    char *buf = s_dbg_rx_buf;
    static const uint8_t test_frame[] = {
        0x01, 0x03, 0x00, 0x00, 0x00, 0x01, 0x84, 0x0A
    };
    module_t *ac;
    int n;

    if (!cmd_is(data, len, "tx") && !(len == 1 && (data[0] == 'F' || data[0] == 'f')))
        return 0;

    ac = gateway_module(0);
    if (ac && ac->sender) {
        int dn = snprintf((char *)buf, sizeof(s_dbg_rx_buf),
                          "[dbg] ac bus busy=%u gap=%u dir=%p need_txc=%u\r\n",
                          (unsigned)ac->bus.busy,
                          (unsigned)ac->bus.gap_ms,
                          (void *)ac->bus.set_dir,
                          (unsigned)ac->bus.need_tx_complete);
        if (dn > 0)
            debug_tx_enqueue_ex((const uint8_t *)buf, (uint16_t)dn, SENDER_PRIO_CMD);

        uint8_t ret = ac_send_test_frame(test_frame,
                                  sizeof(test_frame));
        n = snprintf((char *)buf, sizeof(s_dbg_rx_buf),
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
            debug_tx_enqueue_ex((const uint8_t *)buf, (uint16_t)n, SENDER_PRIO_CMD);
    } else {
        n = snprintf((char *)buf, sizeof(s_dbg_rx_buf),
                     "[test] ac not ready\r\n");
        if (n > 0)
            debug_tx_enqueue_ex((const uint8_t *)buf, (uint16_t)n, SENDER_PRIO_CMD);
    }
    return 1;
}

static int debug_cmd_brand(uint8_t *data, uint16_t len)
{
    char *buf = s_dbg_rx_buf;
    int brand_idx = -1;
    bsp_ac_brand_t brand;
    const char *brand_name;
    module_t *ac;
    int n;

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

    if (brand_idx < 0)
        return 0;

    brand = BSP_AC_MEIDI;
    brand_name = "meidi";
    if (brand_idx == 1) { brand = BSP_AC_TOSHIBA; brand_name = "toshiba"; }
    else if (brand_idx == 2) { brand = BSP_AC_HAIER; brand_name = "haier"; }

    bsp_ac_select(bsp_board_get(), brand);

    static const uint8_t test_frame[] = {
        0x01, 0x03, 0x00, 0x00, 0x00, 0x01, 0x84, 0x0A
    };

    ac = gateway_module(0);
    if (ac && ac->sender) {
        uint8_t ret = ac_send_test_frame(test_frame,
                                  sizeof(test_frame));
        n = snprintf((char *)buf, sizeof(s_dbg_rx_buf),
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
            debug_tx_enqueue_ex((const uint8_t *)buf, (uint16_t)n, SENDER_PRIO_CMD);
    } else {
        n = snprintf((char *)buf, sizeof(s_dbg_rx_buf),
                     "[test] ac not ready\r\n");
        if (n > 0)
            debug_tx_enqueue_ex((const uint8_t *)buf, (uint16_t)n, SENDER_PRIO_CMD);
    }
    return 1;
}

static int debug_cmd_hb(uint8_t *data, uint16_t len)
{
    char *buf = s_dbg_rx_buf;
    int n;

    if (!cmd_is(data, len, "hb") && !cmd_is(data, len, "heartbeat"))
        return 0;

    s_log_heartbeat_enabled = !s_log_heartbeat_enabled;
    n = snprintf((char *)buf, sizeof(s_dbg_rx_buf),
                 "[log] heartbeat %s\r\n",
                 s_log_heartbeat_enabled ? "on" : "off");
    if (n > 0)
        debug_tx_enqueue_ex((const uint8_t *)buf, (uint16_t)n, SENDER_PRIO_CMD);
    return 1;
}

static int debug_cmd_evt(uint8_t *data, uint16_t len)
{
    char *buf = s_dbg_rx_buf;
    int n;

    if (!cmd_is(data, len, "evt") && !cmd_is(data, len, "event"))
        return 0;

    s_log_event_enabled = !s_log_event_enabled;
    n = snprintf((char *)buf, sizeof(s_dbg_rx_buf),
                 "[log] event %s\r\n",
                 s_log_event_enabled ? "on" : "off");
    if (n > 0)
        debug_tx_enqueue_ex((const uint8_t *)buf, (uint16_t)n, SENDER_PRIO_CMD);
    return 1;
}

static int debug_cmd_rxlog(uint8_t *data, uint16_t len)
{
    char *buf = s_dbg_rx_buf;
    int n;

    if (!cmd_is(data, len, "rxlog") && !cmd_is(data, len, "rx"))
        return 0;

    s_log_rx_enabled = !s_log_rx_enabled;
    n = snprintf((char *)buf, sizeof(s_dbg_rx_buf),
                 "[log] rx %s\r\n",
                 s_log_rx_enabled ? "on" : "off");
    if (n > 0)
        debug_tx_enqueue_ex((const uint8_t *)buf, (uint16_t)n, SENDER_PRIO_CMD);
    return 1;
}

static int on_rx_frame(void *ctx, uint8_t *data, uint16_t len)
{
    char *buf = s_dbg_rx_buf;
    int pos;
    (void)ctx;

    if (!g_dbg.base.sender)
        return 1;

    /* 去掉串口助手可能附加的回车/换行/空格 */
    while (len > 0 && (data[len - 1] == '\r' || data[len - 1] == '\n' || data[len - 1] == ' '))
        len--;


    uint8_t handled = 0;
    if (debug_cmd_hb(data, len))         handled = 1;
    else if (debug_cmd_evt(data, len))   handled = 1;
    else if (debug_cmd_rxlog(data, len)) handled = 1;
    else if (debug_cmd_perf(data, len))  handled = 1;
    else if (debug_cmd_stat(data, len))  handled = 1;
    else if (debug_cmd_ack(data, len))   handled = 1;
    else if (debug_cmd_tick(data, len))  handled = 1;
    else if (debug_cmd_idle(data, len))  handled = 1;
    else if (debug_cmd_ctrl(data, len))  handled = 1;
    else if (debug_cmd_state(data, len)) handled = 1;
    else if (debug_cmd_tx(data, len))    handled = 1;
    else if (debug_cmd_brand(data, len)) handled = 1;

    if (handled) {
        /* 让低优先级 send_task / 定时器任务有机会运行，避免 RX 刷屏饿死心跳 */
        taskYIELD();
        return 1;
    }

    /* 未识别命令：HEX 回显 */
    pos = snprintf((char *)buf, sizeof(s_dbg_rx_buf), "[rx]");
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
    debug_tx_enqueue_ex((const uint8_t *)buf, (uint16_t)pos, SENDER_PRIO_CMD);
    return 1;
}

static void on_periodic_send(void *ctx)
{
    static uint16_t alive_div = 0;
    (void)ctx;
    static const char alive[] = "alive\r\n";

    halLedRunBlink();   /* 运行指示灯保持原节奏闪烁 */

    /* 心跳 5s 一跳（默认 poll 200ms，5s/200ms = 25 次），可用 hb 命令开关 */
    if (++alive_div >= 25) {
        alive_div = 0;
        if (s_log_heartbeat_enabled)
            debug_tx_enqueue_ex((const uint8_t *)alive, sizeof(alive) - 1, SENDER_PRIO_NORM);
    }
}

static void on_gateway_cmd(void *ctx, uint8_t cmd, uint8_t val,
                            const gateway_state_t *state)
{
    (void)ctx; (void)cmd; (void)val;

    if (state) {
        g_dbg.base.state = *state;
        log_printf("[dbg] state sync p=%u m=%u t=%u f=%u\r\n",
                   (unsigned)state->power, (unsigned)state->mode,
                   (unsigned)state->set_temp, (unsigned)state->fan);
    }
}

static const event_handler_t debug_evt_table = {
    .on_rx_frame    = on_rx_frame,
    .on_periodic_send = on_periodic_send,
    .on_gateway_cmd = on_gateway_cmd,
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
    .on_event              = debug_ops_on_event,
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

/* Debug send_task 处理 EVENT_DEBUG_TX：真正调用 sender_send */
static uint8_t debug_tx_slot_send(uint8_t idx)
{
    uint8_t ret;

    if (idx >= DEBUG_TX_SLOTS || !g_dbg_tx_slot_used[idx])
        return 1;

    ret = g_dbg.base.sender ?
          sender_send(g_dbg.base.sender,
                      g_dbg_tx_slots[idx].data,
                      g_dbg_tx_slots[idx].len,
                      g_dbg_tx_slots[idx].prio) : 1;

    g_dbg_tx_slot_used[idx] = 0;
    if (g_dbg_tx_free_queue)
        xQueueSend(g_dbg_tx_free_queue, &idx, 0);
    return ret;
}

static void debug_ops_on_event(module_t *m, const event_t *ev)
{
    (void)m;
    if (ev && ev->type == EVENT_DEBUG_TX)
        debug_tx_slot_send(ev->cmd_val);
}

/* 把测试帧通过模块级 API 投递给 AC send_task，由它统一发送 */
static uint8_t ac_send_test_frame(const uint8_t *data, uint16_t len)
{
    module_t *ac = gateway_module(0);
    if (!ac)
        return 1;
    return module_send_frame(ac, data, len, SENDER_PRIO_CMD);
}

/* 把 Debug TX 文本投入 send_queue，由 Debug send_task 统一 sender_send */
static uint8_t debug_tx_enqueue_ex(const uint8_t *data, uint16_t len, uint8_t prio)
{
    uint8_t idx;
    event_t ev;

    if (!g_dbg.base.sender || !g_dbg_tx_free_queue ||
        !data || len == 0 || len > DEBUG_TX_MSG_MAX)
        return 1;

    if (xQueueReceive(g_dbg_tx_free_queue, &idx, 0) != pdPASS)
        return 1;   /* 槽满，丢弃本次日志 */

    g_dbg_tx_slot_used[idx] = 1;
    g_dbg_tx_slots[idx].len  = len;
    g_dbg_tx_slots[idx].prio = prio;
    memcpy(g_dbg_tx_slots[idx].data, data, len);

    memset(&ev, 0, sizeof(ev));
    ev.type    = EVENT_DEBUG_TX;
    ev.cmd_val = idx;

    if (xQueueSend(g_dbg.base.send_queue, &ev, 0) != pdPASS) {
        g_dbg_tx_slot_used[idx] = 0;
        xQueueSend(g_dbg_tx_free_queue, &idx, 0);
        return 1;
    }
    return 0;
}

/* 公共发送：投递到 Debug send_queue，由 send_task 统一发送 */
void log_vprintf(const char *fmt, va_list ap)
{
    uint16_t size;
    char *buf = log_get_buffer(&size);
    int n;

    if (!g_dbg.base.sender)
        return;

    if (log_print_mutex)
        xSemaphoreTake(log_print_mutex, portMAX_DELAY);

    n = vsnprintf(buf, size, fmt, ap);
    if (n > 0)
        debug_tx_enqueue_ex((const uint8_t *)buf, (uint16_t)n, SENDER_PRIO_CMD);

    if (log_print_mutex)
        xSemaphoreGive(log_print_mutex);
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
            debug_tx_enqueue_ex((const uint8_t *)buf, (uint16_t)pos, SENDER_PRIO_CMD);
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
        debug_tx_enqueue_ex((const uint8_t *)buf, (uint16_t)pos, SENDER_PRIO_CMD);
}

/* 调试模块挂接 AC 模块的 RX 日志：AC 模块自身不感知日志 */
static void dbg_module_rx_log(module_t *m, const uint8_t *data, uint16_t len)
{
    (void)m;
    if (s_log_rx_enabled)
        log_hex_dump("ac evt", data, len);
}

/* 网关状态观察者：用于验证发布/订阅链路 */
static void debug_on_state_change(uint8_t module_id,
                                  const gateway_state_t *s,
                                  void *ctx)
{
    (void)ctx;
    log_printf("[gw] module=%u power=%u mode=%u set=%u room=%u fan=%u swing=%u err=%u\r\n",
               (unsigned)module_id,
               (unsigned)s->power, (unsigned)s->mode,
               (unsigned)s->set_temp, (unsigned)s->room_temp,
               (unsigned)s->fan, (unsigned)s->swing,
               (unsigned)s->error_code);
}

void debug_module_start(void)
{
    uint32_t baudrate = 115200;

#ifdef __CH579__
    SetSysClock(CLK_SOURCE_PLL_32MHz);
    DelayMs(1);
#endif

    halLedInit();   /* 运行 LED 初始化 */

    log_print_mutex = xSemaphoreCreateMutex();

    g_dbg.base.ops = &debug_module_ops;
    module_set_handler(&g_dbg.base, &debug_evt_table, NULL);

    /* 物理层装配：固定 UART1，由 debug_phy_init 创建具体对象并注入 */
    if (debug_phy_init(&g_dbg.base.bus, &g_dbg_io) != 0) {
        return;
    }
    g_dbg.base.sender   = g_dbg_io.sender;
    g_dbg.base.receiver = g_dbg_io.receiver;

    module_init(&g_dbg.base, &baudrate);

    /* Debug TX 空闲槽队列：避免用临界区分配槽 */
    g_dbg_tx_free_queue = xQueueCreate(DEBUG_TX_SLOTS, sizeof(uint8_t));
    if (!g_dbg_tx_free_queue)
        return;
    for (uint8_t i = 0; i < DEBUG_TX_SLOTS; i++) {
        uint8_t idx = i;
        xQueueSend(g_dbg_tx_free_queue, &idx, 0);
    }

    gateway_set_module(1, &g_dbg.base);

    /* 注册网关状态观察者，验证状态发布/订阅链路 */
    gateway_on_state_change(debug_on_state_change, NULL);

    /* RX 日志由 debug 模块统一管理：挂到 AC 模块的运行时日志钩子 */
    {
        module_t *ac = gateway_module(0);
        if (ac)
            ac->rx_log = dbg_module_rx_log;
    }

    module_start(&g_dbg.base);

}
