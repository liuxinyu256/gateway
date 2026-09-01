/**
 * ac_test.c —— 测试品牌实现
 *
 * 实现 AC 模块全部事件（只做协议行为，不负责日志打印），
 * cmd 事件会更新 AC 模块状态并上报网关。
 * 事件日志由 ac_module 统一打印。
 */
#include "ac_test.h"
#include "gateway.h"
#include "sender.h"
#include <string.h>

static void test_send_query(ac_module_t *self)
{
    static const uint8_t frame[] = {
        0x01, 0x03, 0x00, 0x00, 0x00, 0x01, 0x84, 0x0A
    };

    if (self && self->base.sender)
        sender_send(self->base.sender, frame, sizeof(frame), SENDER_PRIO_CMD);
}

static void test_on_activate(void *ctx)
{
    (void)ctx;
}

static void test_on_periodic_send(void *ctx)
{
    ac_module_t *self = (ac_module_t *)ctx;
    test_send_query(self);
}

static int test_on_rx_frame(void *ctx, uint8_t *data, uint16_t len)
{
    (void)ctx;
    (void)data;
    (void)len;
    return 1;
}

static void test_on_control_cmd(void *ctx, uint8_t cmd, uint8_t val)
{
    ac_module_t   *self = (ac_module_t *)ctx;
    gateway_state_t s;

    if (!self)
        return;

    if (gateway_module_state_get(0, &s) != 0)
        memset(&s, 0, sizeof(s));

    /* 测试协议：cmd 映射到状态字段 */
    switch (cmd) {
    case 0: s.power = val; break;
    case 1: s.mode = val; break;
    case 2: s.set_temp = val; break;
    case 3: s.room_temp = val; break;
    case 4: s.fan = val; break;
    case 5: s.swing = val; break;
    default: break;
    }

    ac_module_update_state(self, &s);
}

static void test_on_need_ack(void *ctx)
{
    (void)ctx;
}

static void test_on_scan(void *ctx)
{
    ac_module_t *self = (ac_module_t *)ctx;
    test_send_query(self);
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

/* 测试品牌物理层：UART 9600, 8N1, 帧间隙 5ms */
static const uart_phy_cfg_t ac_test_uart_cfg = {
    .baudrate   = 9600,
    .data_bits  = 8,
    .stop_bits  = 1,
    .parity     = 0,
    .receiver_timeout_ticks = 5,
};

static const ac_phy_cfg_t ac_test_phy_cfg = {
    .phy_type = AC_PHY_RS485,
    .cfg      = &ac_test_uart_cfg,
};

/* 测试品牌配置：能力全部放开，覆盖所有模型 */
const ac_brand_config_t ac_test_cfg = {
    .brand_id = ac_test,
    .phy_cfg = &ac_test_phy_cfg,
    .evt_table = &ac_test_evt,
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
