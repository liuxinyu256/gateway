#include <stdio.h>
#include <string.h>
#include "gateway.h"
#include "ac_module.h"
#include "sender.h"
#include "encoder.h"
#include "decoder.h"
#include "fake_freertos.h"

/* ---- 模拟编码器：字节直接进入 TX 捕获缓冲 ---- */
static uint8_t  g_tx_buf[128];
static uint16_t g_tx_len;

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
static sender_t  sim_sender;

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
    sender_send(&sim_sender, f, sizeof(f), SENDER_PRIO_NORM);
    sim_drain_tx(&sim_sender);
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
    (void)ctx;
    uint8_t f[6] = { 0xAA, 0x01, cmd, val, 0x00, 0x55 };
    f[4] = (uint8_t)~(0xAA + 0x01 + cmd + val);

    g_tx_len = 0;
    sender_send(&sim_sender, f, sizeof(f), SENDER_PRIO_CMD);
    sim_drain_tx(&sim_sender);
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
                          test_brand.receiver_timeout_ticks, NULL,
                          rx_ring_buf, sizeof(rx_ring_buf));
    receiver_set_bus(&rx_timeout.base, &m->bus);
    /* 解码器通过回调把字节喂给接收器 */
    decoder_set_rx_callback(&sim_decoder, sim_decoder_to_receiver,
                            &rx_timeout.base);
    m->receiver = &rx_timeout.base;

    /* 发送器由上层创建并注入 */
    sender_cfg_t sender_cfg = {
        .encoder = &sim_encoder,
        .bus     = &m->bus,
    };
    sender_init(&sim_sender, &sender_cfg);
    m->sender = &sim_sender;

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

    /* 2. 模拟收到一帧 → decoder → receiver → frame_done → receive_queue */
    printf("-- rx frame --\n");
    receiver_t *rx = m->receiver;
    uint8_t frame[] = { 0xAA, 0x01, 0x20, 0x30, 0x00, 0x55 };
    for (size_t i = 0; i < sizeof(frame); i++)
        decoder_feed_byte(&sim_decoder, frame[i]);
    rx->frame_len = (uint16_t)sizeof(frame);
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

    printf("[OK] closed-loop smoke test passed\n");
    return 0;
}
