/**
 * ac_test.c —— 测试品牌实现
 *
 * 实现 AC 模块全部事件，便于验证模块基本功能。
 * 正式品牌协议开发后可移除。
 */
#include "ac_test.h"
#include "gateway.h"
#include "sender.h"
#include <stdio.h>

static void test_on_activate(void *ctx)
{
    (void)ctx;
}

static void test_on_periodic_send(void *ctx)
{
    ac_module_t *self = (ac_module_t *)ctx;
    static const uint8_t frame[] = {
        0x01, 0x03, 0x00, 0x00, 0x00, 0x01, 0x84, 0x0A
    };

    if (self && self->base.sender)
        sender_send(self->base.sender, frame, sizeof(frame), SENDER_PRIO_CMD);
}

static int test_on_rx_frame(void *ctx, uint8_t *data, uint16_t len)
{
    (void)ctx;
    module_t *dbg = gateway_module(1);
    char buf[64];   /* 小缓冲，分段发送，避免大局部数组 */
    int pos;

    if (!dbg || !dbg->sender || !data || !len)
        return 1;

    pos = snprintf(buf, sizeof(buf), "[ac rx]");
    for (uint16_t i = 0; i < len; i++) {
        if (pos + 4 >= (int)sizeof(buf)) {
            sender_send(dbg->sender, (const uint8_t *)buf,
                        (uint16_t)pos, SENDER_PRIO_CMD);
            pos = 0;
        }
        pos += snprintf(buf + pos, sizeof(buf) - (size_t)pos,
                        " %02X", data[i]);
    }
    if (pos + 2 < (int)sizeof(buf))
        pos += snprintf(buf + pos, sizeof(buf) - (size_t)pos, "\r\n");
    if (pos > 0)
        sender_send(dbg->sender, (const uint8_t *)buf,
                    (uint16_t)pos, SENDER_PRIO_CMD);
    return 1;
}

static void test_on_control_cmd(void *ctx, uint8_t cmd, uint8_t val)
{
    (void)ctx;
    (void)cmd;
    (void)val;
}

static void test_on_need_ack(void *ctx)
{
    (void)ctx;
}

static void test_on_scan(void *ctx)
{
    /* 扫描动作：先发一帧查询 */
    test_on_periodic_send(ctx);
}

static void test_on_timeout(void *ctx)
{
    (void)ctx;
}

static const event_handler_t ac_test_evt = {
    .on_activate       = test_on_activate,
    .on_periodic_send  = test_on_periodic_send,
    .on_rx_frame       = test_on_rx_frame,
    .on_control_cmd    = test_on_control_cmd,
    .on_need_ack       = test_on_need_ack,
    .on_scan           = test_on_scan,
    .on_timeout        = test_on_timeout,
};

/* 测试品牌配置：能力全部放开，覆盖所有模型 */
const ac_brand_config_t ac_test_cfg = {
    .brand_id = ac_test,
    .evt_table = &ac_test_evt,
    .receiver_timeout_ticks = 5,
    .ability = {
        .mode_caps = (1u << MODE_COOL) | (1u << MODE_HEAT) |
                     (1u << MODE_FAN)  | (1u << MODE_DRY) |
                     (1u << MODE_AUTO),
        .fan_caps  = (1u << FAN_AUTO) | (1u << FAN_1) | (1u << FAN_2) |
                     (1u << FAN_3)    | (1u << FAN_4) | (1u << FAN_5) |
                     (1u << FAN_6),
        .swing_caps = (1u << SWING_OFF) | (1u << SWING_UD) |
                      (1u << SWING_LR)  | (1u << SWING_ALL),
        .temp_min  = 16,
        .temp_max  = 30,
        .temp_step = 10,
        .features  = AC_FEAT_TIMER | AC_FEAT_SLEEP | AC_FEAT_HEALTH,
    },
};
