/**
 * ac_test.c —— 测试品牌实现（自定 Modbus 测试协议）
 *
 * 用 Modbus RTU 保持寄存器模拟一台 AC：
 *   0x0000 power
 *   0x0001 mode
 *   0x0002 set_temp
 *   0x0003 fan
 *   0x0004 swing
 *   0x0005 room_temp（只读）
 *   0x0006 error_code（只读）
 *
 * 品牌层只写协议逻辑：
 *   - rx_parse：帧 -> 品牌事件
 *   - state_machine：品牌事件 + 状态 -> 帧 + 下次定时
 * 不负责发送，也不接触模块/任务/队列。
 */
#include "ac_test.h"
#include <string.h>

#define AC_TEST_TX_ENABLE      1   /* 1=允许发送，0=临时关闭发送 */
#define AC_TEST_SLAVE_ADDR     0x01
#define AC_TEST_REG_POWER      0x0000
#define AC_TEST_REG_MODE       0x0001
#define AC_TEST_REG_SET_TEMP   0x0002
#define AC_TEST_REG_FAN        0x0003
#define AC_TEST_REG_SWING      0x0004
#define AC_TEST_REG_ROOM_TEMP  0x0005
#define AC_TEST_REG_ERROR      0x0006
#define AC_TEST_READ_COUNT     7
#define AC_TEST_WRITE_COUNT    5   /* 只写可控字段: power/mode/set_temp/fan/swing */

/* 品牌自定义 RX 事件（从公共 AC_EV_RX_BASE 开始） */
enum {
    AC_TEST_EV_RX_RESP      = AC_EV_RX_BASE,
    AC_TEST_EV_RX_WRITE_ACK,
};

static uint16_t test_crc16(const uint8_t *data, uint16_t len)
{
    uint16_t crc = 0xFFFF;
    uint16_t i;
    uint8_t bit;

    for (i = 0; i < len; i++) {
        crc ^= data[i];
        for (bit = 0; bit < 8; bit++) {
            if (crc & 1)
                crc = (crc >> 1) ^ 0xA001;
            else
                crc >>= 1;
        }
    }
    return crc;
}

/* 把一个保持寄存器值写回 ac_state_t 对应字段 */
static void test_state_from_reg(ac_state_t *s, uint16_t reg, uint16_t value)
{
    if (!s)
        return;

    switch (reg) {
    case AC_TEST_REG_POWER:
        s->power = (uint8_t)value;
        break;
    case AC_TEST_REG_MODE:
        s->mode = (uint8_t)value;
        break;
    case AC_TEST_REG_SET_TEMP:
        s->set_temp = (uint8_t)value;
        break;
    case AC_TEST_REG_FAN:
        s->fan = (uint8_t)value;
        break;
    case AC_TEST_REG_SWING:
        s->swing = (uint8_t)value;
        break;
    case AC_TEST_REG_ROOM_TEMP:
        s->room_temp = (uint8_t)value;
        break;
    case AC_TEST_REG_ERROR:
        s->error_code = (uint8_t)value;
        break;
    default:
        break;
    }
}

/* 读保持寄存器 0x0000 起 7 个寄存器，一次拿回完整 AC 状态 */
static uint16_t test_build_query(uint8_t *buf, uint16_t max)
{
    uint16_t crc;

    if (!AC_TEST_TX_ENABLE)
        return 0;
    if (!buf || max < 8)
        return 0;

    buf[0] = AC_TEST_SLAVE_ADDR;
    buf[1] = 0x03;                      /* 读保持寄存器 */
    buf[2] = (uint8_t)(AC_TEST_REG_POWER >> 8);
    buf[3] = (uint8_t)(AC_TEST_REG_POWER & 0xFF);
    buf[4] = 0x00;
    buf[5] = AC_TEST_READ_COUNT;

    crc = test_crc16(buf, 6);
    buf[6] = (uint8_t)(crc & 0xFF);
    buf[7] = (uint8_t)(crc >> 8);
    return 8;
}

/* 写多个保持寄存器 0x0000~0x0004：把可控 AC 状态一次写进模拟器 */
static uint16_t test_build_control(const ac_state_t *state,
                                   uint8_t *buf, uint16_t max)
{
    const uint16_t data_len = AC_TEST_WRITE_COUNT * 2;
    const uint16_t total    = 7 + data_len + 2;   /* 地址+功能+寄存器+数量+字节数+数据+CRC */
    uint8_t *p;
    uint16_t crc;

    if (!AC_TEST_TX_ENABLE)
        return 0;
    if (!state || !buf || max < total)
        return 0;

    buf[0] = AC_TEST_SLAVE_ADDR;
    buf[1] = 0x10;                      /* 写多个保持寄存器 */
    buf[2] = (uint8_t)(AC_TEST_REG_POWER >> 8);
    buf[3] = (uint8_t)(AC_TEST_REG_POWER & 0xFF);
    buf[4] = 0x00;
    buf[5] = AC_TEST_WRITE_COUNT;
    buf[6] = (uint8_t)data_len;

    p = &buf[7];
    *p++ = 0x00; *p++ = state->power;
    *p++ = 0x00; *p++ = state->mode;
    *p++ = 0x00; *p++ = state->set_temp;
    *p++ = 0x00; *p++ = state->fan;
    *p++ = 0x00; *p++ = state->swing;

    crc = test_crc16(buf, 7 + data_len);
    buf[7 + data_len]     = (uint8_t)(crc & 0xFF);
    buf[7 + data_len + 1] = (uint8_t)(crc >> 8);
    return total;
}

/* 接收解析：帧 -> 品牌事件；只解析/更新 ac_state_t，不组回复帧 */
static uint8_t test_parse_rx(const uint8_t *data, uint16_t len,
                             ac_state_t *out)
{
    uint16_t crc;

    if (!data || !out || len < 5)
        return AC_EV_NONE;

    if (data[0] != AC_TEST_SLAVE_ADDR)
        return AC_EV_NONE;

    /* CRC 校验：低字节在前 */
    crc = test_crc16(data, len - 2);
    if (data[len - 2] != (uint8_t)(crc & 0xFF) ||
        data[len - 1] != (uint8_t)(crc >> 8))
        return AC_EV_NONE;

    if (data[1] == 0x03) {
        uint8_t byte_count = data[2];
        const uint8_t *regs;

        if (byte_count != AC_TEST_READ_COUNT * 2)
            return AC_EV_NONE;
        if (len != (uint16_t)(3 + byte_count + 2))
            return AC_EV_NONE;

        regs = &data[3];
        out->power      = (uint8_t)((regs[0] << 8) | regs[1]);
        out->mode       = (uint8_t)((regs[2] << 8) | regs[3]);
        out->set_temp   = (uint8_t)((regs[4] << 8) | regs[5]);
        out->fan        = (uint8_t)((regs[6] << 8) | regs[7]);
        out->swing      = (uint8_t)((regs[8] << 8) | regs[9]);
        out->room_temp  = (uint8_t)((regs[10] << 8) | regs[11]);
        out->error_code = (uint8_t)((regs[12] << 8) | regs[13]);
        return AC_TEST_EV_RX_RESP;
    }

    if (data[1] == 0x06) {
        uint16_t reg;
        uint16_t value;

        if (len != 8)
            return AC_EV_NONE;

        reg   = (uint16_t)((data[2] << 8) | data[3]);
        value = (uint16_t)((data[4] << 8) | data[5]);
        test_state_from_reg(out, reg, value);
        return AC_TEST_EV_RX_WRITE_ACK;
    }

    if (data[1] == 0x10) {
        /* 0x10 回显不含寄存器值；状态以轮询读回为准 */
        if (len != 8)
            return AC_EV_NONE;
        return AC_TEST_EV_RX_WRITE_ACK;
    }

    return AC_EV_NONE;
}

/* 发送状态机：事件 + 当前状态 -> 要发的帧 + 下次定时 */
static uint16_t test_state_machine(uint8_t event,
                                   const ac_state_t *state,
                                   uint8_t *tx, uint16_t tx_max,
                                   uint16_t *next_period_ms)
{
    if (next_period_ms)
        *next_period_ms = 2000;

    switch (event) {
    case AC_EV_POLL:
        return test_build_query(tx, tx_max);

    case AC_EV_STATE_SYNC:
        return test_build_control(state, tx, tx_max);

    case AC_TEST_EV_RX_RESP:
    case AC_TEST_EV_RX_WRITE_ACK:
        /* 收到应答/写回执：不需要额外发帧，保持周期轮询 */
        return 0;

    default:
        return 0;
    }
}

static const ac_protocol_ops_t ac_test_protocol_ops = {
    .on_scan        = test_build_query,
    .state_machine  = test_state_machine,
    .rx_parse       = test_parse_rx,
    .poll_period_ms = 2000,
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
    .phy_cfg  = &ac_test_phy_cfg,
    .protocol_ops = &ac_test_protocol_ops,
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
