#include <stdio.h>
#include <string.h>
#include "gateway.h"
#include "ac_module.h"
#include "sender.h"
#include "sender_complete_poll.h"
#include "encoder.h"
#include "decoder.h"
#include "fake_freertos.h"

/* ---- 模拟编码器：字节直接进入 TX 捕获缓冲 ---- */
static uint8_t  g_tx_buf[128];
static uint16_t g_tx_len;

/* ---- 状态同步验证捕获 ---- */
static gateway_state_t g_synced_state;
static int g_sync_count;
static gateway_state_t g_dbg_synced_state;
static int g_dbg_sync_count;

static void sim_write(uint8_t byte)
{
    if (g_tx_len < sizeof(g_tx_buf))
        g_tx_buf[g_tx_len++] = byte;
}

static uint8_t sim_encoder_configure(encoder_t *e, const void *cfg)
{
    (void)e; (void)cfg;
    return 0;
}

static uint8_t sim_encoder_encode_byte(encoder_t *e, uint8_t byte)
{
    (void)e;
    sim_write(byte);
    return 0;
}

static void sim_encoder_tx_enable(encoder_t *e) { (void)e; }
static void sim_encoder_tx_disable(encoder_t *e) { (void)e; }
static uint8_t sim_encoder_tx_ready(encoder_t *e) { (void)e; return 1; }
static uint8_t sim_encoder_tx_complete(encoder_t *e) { (void)e; return 1; }

static const encoder_ops_t sim_encoder_ops = {
    .configure    = sim_encoder_configure,
    .encode_byte  = sim_encoder_encode_byte,
    .tx_enable    = sim_encoder_tx_enable,
    .tx_disable   = sim_encoder_tx_disable,
    .tx_ready     = sim_encoder_tx_ready,
    .tx_complete  = sim_encoder_tx_complete,
};

static encoder_t sim_encoder = { .ops = &sim_encoder_ops };
static sender_poll_t sim_sender;
static uint8_t sim_cmd_ring_buf[256];
static uint8_t sim_norm_ring_buf[256];

/* ---- 模拟解码器：字节原样转发给接收器 ---- */
static int sim_decoder_init(decoder_t *d, const void *cfg)
{
    (void)d; (void)cfg;
    return 0;
}

static void sim_decoder_feed_byte(decoder_t *d, uint8_t byte)
{
    if (d && d->rx_cb)
        d->rx_cb(byte, d->rx_ctx);
}

static const decoder_ops_t sim_decoder_ops = {
    .init            = sim_decoder_init,
    .set_rx_callback = NULL, /* decoder_set_rx_callback 已在基类保存 */
    .feed_byte       = sim_decoder_feed_byte,
    .feed_sample     = NULL,
};

static decoder_t sim_decoder = { .ops = &sim_decoder_ops };

/* 模拟解码器回调: 字节原样喂给接收器 */
static void sim_decoder_to_receiver(uint8_t byte, void *ctx)
{
    receiver_put_byte((receiver_t *)ctx, byte);
}

/* 模拟 ISR 链: 把当前帧按字节发完 */
static void sim_drain_tx(sender_t *s)
{
    sender_pump(s);

    while (s->sending)
        sender_isr(s);
}

/* ---- 测试用品牌事件表 ---- */
static void test_on_periodic(void *ctx)
{
    (void)ctx;
    uint8_t f[] = { 0xAA, 0x01, 0x20, 0x00, 0xDE, 0x55 };

    g_tx_len = 0;
    sender_send(&sim_sender.base, f, sizeof(f), SENDER_PRIO_NORM);
    sim_drain_tx(&sim_sender.base);
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

static void test_on_control(void *ctx, uint8_t cmd, uint8_t val,
                            const gateway_state_t *state)
{
    ac_module_t *self = (ac_module_t *)ctx;

    /* 完整状态同步：镜像网关广播的真相源，并记录供测试断言 */
    if (state) {
        g_synced_state = *state;
        g_sync_count++;
        if (self)
            self->base.state = *state;
        return;
    }

    uint8_t f[6] = { 0xAA, 0x01, cmd, val, 0x00, 0x55 };
    f[4] = (uint8_t)~(0xAA + 0x01 + cmd + val);

    g_tx_len = 0;
    sender_send(&sim_sender.base, f, sizeof(f), SENDER_PRIO_CMD);
    sim_drain_tx(&sim_sender.base);
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

static const event_handler_t test_table = {
    .on_periodic_send = test_on_periodic,
    .on_rx_frame      = test_on_rx_frame,
    .on_gateway_cmd    = test_on_control,
    .on_need_ack      = test_on_need_ack,
    .on_scan          = test_on_scan,
};

static const uart_phy_cfg_t test_uart_cfg = {
    .baudrate   = 9600,
    .data_bits  = 8,
    .stop_bits  = 1,
    .parity     = 0,
    .receiver_timeout_ticks = 5,
};

static const ac_phy_cfg_t test_phy_cfg = {
    .phy_type = AC_PHY_UART,
    .cfg      = &test_uart_cfg,
};

static const ac_brand_config_t test_brand = {
    .brand_id = 1,
    .phy_cfg  = &test_phy_cfg,
    .evt_table = &test_table,
    .ability  = { 0 },
};

static const ac_brand_config_t *const test_brand_table[] = {
    [0] = NULL,
    [1] = &test_brand,
};

/* ---- 占位/源模块：模拟 Debug 槽位和 BLE 模块槽位 ---- */
static uint8_t dummy_ops_init(module_t *m, void *cfg)
{
    (void)cfg;
    return module_base_init(m, 9600);
}

static uint8_t ble_src_ops_init(module_t *m, void *cfg)
{
    (void)cfg;
    return module_base_init(m, 9600);
}

static const module_ops_t dummy_ops = {
    .init = dummy_ops_init,
};

static const module_ops_t ble_src_ops = {
    .init = ble_src_ops_init,
};

/* Debug-like 目标模块：验证网关也会把完整状态同步给非 AC 模块 */
static void dbg_on_gateway_cmd(void *ctx, uint8_t cmd, uint8_t val,
                               const gateway_state_t *state)
{
    (void)cmd; (void)val;
    if (state) {
        g_dbg_synced_state = *state;
        g_dbg_sync_count++;
        if (ctx)
            ((module_t *)ctx)->state = *state;
    }
}

static const event_handler_t dbg_test_table = {
    .on_gateway_cmd = dbg_on_gateway_cmd,
};

int main(void)
{
    fake_timer_reset();
    gateway_init();

    static ac_module_t ac = { .base.ops = &ac_module_ops };
    static timer_t rx_timer = { 0 };
    static receiver_timeout_t rx_timeout;
    static uint8_t rx_ring_buf[128];
    static ac_init_cfg_t cfg = {
        .baudrate    = 9600,
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

    /* 接收器实例由上层创建并注入 */
    receiver_timeout_init(&rx_timeout, &rx_timer,
                          ((const uart_phy_cfg_t *)test_brand.phy_cfg->cfg)->receiver_timeout_ticks,
                          NULL, rx_ring_buf, sizeof(rx_ring_buf));
    receiver_set_bus(&rx_timeout.base, &m->bus);
    /* 解码器通过回调把字节喂给接收器 */
    decoder_set_rx_callback(&sim_decoder, sim_decoder_to_receiver,
                            &rx_timeout.base);
    m->receiver = &rx_timeout.base;

    /* 发送器由上层创建并注入 */
    sender_cfg_t sender_cfg = {
        .encoder        = &sim_encoder,
        .bus            = &m->bus,
        .cmd_ring_buf   = sim_cmd_ring_buf,
        .cmd_ring_size  = sizeof(sim_cmd_ring_buf),
        .norm_ring_buf  = sim_norm_ring_buf,
        .norm_ring_size = sizeof(sim_norm_ring_buf),
    };
    sender_poll_init(&sim_sender, &sender_cfg);
    m->sender = &sim_sender.base;

    if (module_init(m, &cfg) != 0) {
        printf("[FAIL] module_init\n");
        return 1;
    }
    ac_module_register(&ac, &test_brand);
    module_start(m);
    gateway_set_module(0, m);

    /* 1. 控制命令 → send_queue → on_gateway_cmd → sender → TX 字节 */
    printf("-- send cmd --\n");
    module_send_gateway_cmd(m, 1, 2);
    module_poll(m);
    printf("  TX:");
    for (uint16_t i = 0; i < g_tx_len; i++)
        printf(" %02X", g_tx_buf[i]);
    printf("\n");

    /* 2. 模拟收到一帧 → decoder → receiver → frame_done → receive_queue */
    printf("-- rx frame --\n");
    receiver_t *rx = m->receiver;
    uint8_t frame[] = { 0xAA, 0x01, 0x20, 0x30, 0x00, 0x55 };
    for (size_t i = 0; i < sizeof(frame); i++)
        decoder_feed_byte(&sim_decoder, frame[i]);
    receiver_push_frame(rx, (uint16_t)sizeof(frame));
    if (rx->on_frame_finish)
        rx->on_frame_finish(rx, (uint16_t)sizeof(frame));
    bus_on_rx_complete(&m->bus);   /* 模拟超时封包完成 → 总线空闲 */
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

    /* 4. 状态同步：BLE-like 源模块(module 2)更新 → gateway → AC(module 0)镜像 */
    printf("-- state sync --\n");

    /* Debug-like 模块占槽位 1，BLE-like 源模块占槽位 2 */
    static module_t dbg_target = { .ops = &dummy_ops };
    if (module_init(&dbg_target, NULL) != 0) {
        printf("[FAIL] dbg_target module_init\n");
        return 1;
    }
    module_set_handler(&dbg_target, &dbg_test_table, &dbg_target);
    gateway_set_module(1, &dbg_target);

    static module_t ble_src = { .ops = &ble_src_ops };
    if (module_init(&ble_src, NULL) != 0) {
        printf("[FAIL] ble_src module_init\n");
        return 1;
    }
    gateway_set_module(2, &ble_src);

    g_sync_count = 0;
    g_dbg_sync_count = 0;
    memset(&g_synced_state, 0, sizeof(g_synced_state));
    memset(&g_dbg_synced_state, 0, sizeof(g_dbg_synced_state));

    gateway_state_t desired;
    memset(&desired, 0, sizeof(desired));
    desired.power     = 1;
    desired.mode      = MODE_COOL;
    desired.fan       = FAN_3;
    desired.set_temp  = 25;
    desired.room_temp = 30;
    desired.swing     = SWING_UD;

    module_update_state(&ble_src, &desired);
    module_poll(m);            /* 处理网关状态事件，把 EVENT_GATEWAY_CMD 投递给 AC/Debug */
    module_poll(&dbg_target);  /* Debug-like send_task 处理 EVENT_GATEWAY_CMD */
    module_poll(m);            /* AC send_task 处理 EVENT_GATEWAY_CMD */

    if (g_sync_count != 1) {
        printf("[FAIL] AC on_gateway_cmd state sync not called (count=%d)\n",
               g_sync_count);
        return 1;
    }
    if (memcmp(&ac.base.state, &desired, sizeof(desired)) != 0) {
        printf("[FAIL] AC module state not mirrored\n");
        return 1;
    }
    if (g_dbg_sync_count != 1) {
        printf("[FAIL] Debug-like on_gateway_cmd state sync not called (count=%d)\n",
               g_dbg_sync_count);
        return 1;
    }
    if (memcmp(&dbg_target.state, &desired, sizeof(desired)) != 0) {
        printf("[FAIL] Debug-like module state not mirrored\n");
        return 1;
    }

    printf("  AC state synced: p=%u m=%u f=%u t=%u room=%u swing=%u\n",
           (unsigned)ac.base.state.power,
           (unsigned)ac.base.state.mode,
           (unsigned)ac.base.state.fan,
           (unsigned)ac.base.state.set_temp,
           (unsigned)ac.base.state.room_temp,
           (unsigned)ac.base.state.swing);
    printf("  Debug state synced: p=%u m=%u f=%u t=%u room=%u swing=%u\n",
           (unsigned)dbg_target.state.power,
           (unsigned)dbg_target.state.mode,
           (unsigned)dbg_target.state.fan,
           (unsigned)dbg_target.state.set_temp,
           (unsigned)dbg_target.state.room_temp,
           (unsigned)dbg_target.state.swing);

    printf("[OK] closed-loop smoke test passed\n");
    return 0;
}
