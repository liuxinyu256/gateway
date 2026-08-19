#include <stdio.h>
#include <string.h>
#include "gateway.h"
#include "ac_module.h"
#include "fake_freertos.h"

/* ---- 模拟串口 TX 捕获 ---- */
static uint8_t  g_tx_buf[128];
static uint16_t g_tx_len;

static void sim_write(uint8_t byte)
{
    if (g_tx_len < sizeof(g_tx_buf))
        g_tx_buf[g_tx_len++] = byte;
}

/* 模拟 UART THR_EMPTY 中断链: 把 sender 环形队列里的字节全部发完 */
static void sim_drain_tx(module_t *m)
{
    while (!ring_empty(&m->sender.ring))
        sender_on_thr_empty(&m->sender);
}

/* ---- 测试用品牌事件表 ---- */
static void test_on_periodic(void *ctx)
{
    ac_module_t *ac = (ac_module_t *)ctx;
    uint8_t f[] = { 0xAA, 0x01, 0x20, 0x00, 0xDE, 0x55 };

    g_tx_len = 0;
    sender_send(&ac->mod->sender, f, sizeof(f));
    sim_drain_tx(ac->mod);
    printf("  [periodic] query frame -> %u bytes\n", g_tx_len);
}

static int test_on_rx_frame(void *ctx, uint8_t *data, uint16_t len)
{
    (void)ctx;
    printf("  [rx] %u bytes:", len);
    for (uint16_t i = 0; i < len; i++)
        printf(" %02X", data[i]);
    printf("\n");
    return 1;
}

static void test_on_control(void *ctx, uint8_t cmd, uint8_t val)
{
    ac_module_t *ac = (ac_module_t *)ctx;
    uint8_t f[6] = { 0xAA, 0x01, cmd, val, 0x00, 0x55 };
    f[4] = (uint8_t)~(0xAA + 0x01 + cmd + val);

    g_tx_len = 0;
    sender_send(&ac->mod->sender, f, sizeof(f));
    sim_drain_tx(ac->mod);
    printf("  [control] cmd=%u val=%u -> %u bytes\n", cmd, val, g_tx_len);
}

static void test_on_need_ack(void *ctx)
{
    (void)ctx;
    printf("  [need_ack]\n");
}

static void test_on_scan(void *ctx)
{
    (void)ctx;
    printf("  [scan]\n");
}

static void test_on_timeout(void *ctx)
{
    (void)ctx;
    printf("  [timeout]\n");
}

static const event_handler_t test_table = {
    .on_periodic_send = test_on_periodic,
    .on_rx_frame      = test_on_rx_frame,
    .on_control_cmd   = test_on_control,
    .on_need_ack      = test_on_need_ack,
    .on_scan          = test_on_scan,
    .on_timeout       = test_on_timeout,
};

static const ac_brand_config_t test_brand = {
    .brand_id             = 1,
    .evt_table            = &test_table,
    .receiver_timeout_ticks = 5,
    .ability              = { 0 },
};

static const ac_brand_config_t *const test_brand_table[] = {
    [0] = NULL,
    [1] = &test_brand,
};

int main(void)
{
    fake_timer_reset();
    gateway_init();

    static ac_module_t ac = { .base.ops = &ac_module_ops };
    static timer_t rx_timer = { .id = 0 };
    static receiver_timeout_t rx_timeout;
    static ac_init_cfg_t cfg = {
        .baudrate    = 9600,
        .write_byte  = sim_write,
        .brand_table = test_brand_table,
        .brand_count = 2,
    };

    module_t *m = &ac.base;

    printf("=== Gateway Simulator v1.1 ===\n");

#ifdef FAKE_FREERTOS
    /* PC 模拟：绑定软件定时器 0 */
    timer_sw_bind(&rx_timer, 0);
#else
    /* 真实硬件：独占硬件定时器 0 */
    if (timer_hw_create(&rx_timer, 0) != 0) {
        printf("[FAIL] timer_hw_create\n");
        return 1;
    }
#endif

    /* 接收器实例由上层创建并注入基类指针 */
    receiver_timeout_init(&rx_timeout, &rx_timer,
                          test_brand.receiver_timeout_ticks, NULL,
                          ac.rx_ring_buf, sizeof(ac.rx_ring_buf));
    m->rx = &rx_timeout.base;

    if (module_init(m, &cfg) != 0) {
        printf("[FAIL] module_init\n");
        return 1;
    }
    ac_module_register(&ac, &test_brand);
    module_start(m);

    /* 1. 控制命令 → send_queue → on_control_cmd → sender → TX 字节 */
    printf("-- send cmd --\n");
    module_send_cmd(m, 1, 2);
    module_poll(m);
    printf("  TX:");
    for (uint16_t i = 0; i < g_tx_len; i++)
        printf(" %02X", g_tx_buf[i]);
    printf("\n");

    /* 2. 模拟收到一帧 → receiver → frame_done → on_rx_frame */
    printf("-- rx frame --\n");
    receiver_t *rx = m->rx;
    uint8_t frame[] = { 0xAA, 0x01, 0x20, 0x30, 0x00, 0x55 };
    for (size_t i = 0; i < sizeof(frame); i++)
        receiver_put_byte(rx, frame[i]);
    rx->frame_len = (uint16_t)sizeof(frame);
    if (rx->on_frame_finish)
        rx->on_frame_finish(rx, (uint16_t)sizeof(frame));
    module_poll(m);

    /* 3. 轮询定时器 → on_periodic_send → 查询帧 */
    printf("-- periodic --\n");
    if (ac.base.poll_timer) {
        fake_timer_advance(201);
        fake_timer_fire(ac.base.poll_timer);
        module_poll(m);
    }
    printf("  TX:");
    for (uint16_t i = 0; i < g_tx_len; i++)
        printf(" %02X", g_tx_buf[i]);
    printf("\n");

    printf("[OK] closed-loop smoke test passed\n");
    return 0;
}
