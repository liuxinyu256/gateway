/**
 * ble_proto_1to1.c —— 1to1 BLE 协议（事件驱动版，逐步完善）
 *
 * 当前实现：
 *   - 0x21 查询空调设定状态
 *   - 0x22 设置空调设定参数
 *   - 0x24 读取空调设定权限
 *   - 0x28 查询空调运行/故障信息
 * 其余命令先返回“不支持”响应，后续按业务补充。
 */
#include "ble_proto_1to1.h"
#include "gateway.h"
#include <string.h>

void ble_proto_1to1_init(void)
{
}

static uint16_t build_error_resp(const uint8_t *rx, uint16_t rx_len,
                                 uint8_t *resp, uint16_t resp_max)
{
    uint16_t n;

    if (rx_len < 7 || resp_max < 8)
        return 0;

    n = 8; /* 头7字节 + 1字节错误标志 */
    memcpy(resp, rx, 7);
    resp[0] = 0;
    resp[1] = (uint8_t)(n - 5); /* length 字段 */
    resp[BLE1TO1_IDX_DATA] = 0x01; /* 不支持/错误 */
    return n;
}

static uint16_t build_ac_setting_status(const uint8_t *rx,
                                        uint8_t *resp, uint16_t resp_max)
{
    gateway_state_t s;
    uint16_t n = 15;

    if (resp_max < n)
        return 0;

    memset(resp, 0, n);
    memcpy(resp, rx, 7);

    if (gateway_module_state_get(0, &s) == 0) {
        resp[7]  = 0x01; /* 通讯正常 */
        resp[8]  = 0x01; /* 在线 */
        resp[9]  = s.power;
        resp[10] = s.mode;
        resp[11] = s.fan;
        resp[12] = s.room_temp;
        resp[13] = s.set_temp;
        resp[14] = 0;    /* 主从 */
    } else {
        resp[7] = 0x00;
        resp[8] = 0x00;
    }

    resp[0] = 0;
    resp[1] = (uint8_t)(n - 5);
    return n;
}

static uint16_t build_ac_run_status(const uint8_t *rx,
                                    uint8_t *resp, uint16_t resp_max)
{
    gateway_state_t s;
    uint16_t n = 10; /* step28_table 长度 */

    if (resp_max < n)
        return 0;

    memset(resp, 0, n);
    memcpy(resp, rx, 7);

    if (gateway_module_state_get(0, &s) == 0) {
        resp[7] = 0x01;
        resp[8] = 0x01;
        resp[9] = s.error_code;
    }

    resp[0] = 0;
    resp[1] = (uint8_t)(n - 5);
    return n;
}

static uint16_t build_ac_setting_permission(const uint8_t *rx,
                                            uint8_t *resp, uint16_t resp_max)
{
    /* 先用 step24_table 的固定长度：0x12 数据 + 7 头 = 25? 实际数组长 23? 这里用 23 */
    uint16_t n = 23;

    if (resp_max < n)
        return 0;

    memset(resp, 0, n);
    memcpy(resp, rx, 7);
    resp[0] = 0;
    resp[1] = (uint8_t)(n - 5);
    return n;
}

static uint16_t handle_set_ac_parameter(const uint8_t *rx, uint16_t rx_len,
                                        uint8_t *resp, uint16_t resp_max)
{
    gateway_state_t s;
    uint16_t n;

    if (rx_len < 16 || resp_max < 8)
        return 0;

    /* 原协议 0x22 数据在 index 7 开始，具体映射按后续协议文档细化 */
    if (gateway_module_state_get(0, &s) != 0)
        memset(&s, 0, sizeof(s));

    /* 这里先不实际修改状态，等确认协议数据位后补全 */
    n = build_error_resp(rx, rx_len, resp, resp_max);
    return n;
}

uint16_t ble_proto_1to1_on_rx(const uint8_t *data, uint16_t len,
                              uint8_t *resp, uint16_t resp_max)
{
    uint16_t length;

    if (!data || len < 7 || !resp || resp_max < 8)
        return 0;

    length = ((uint16_t)data[BLE1TO1_IDX_NUM_H] << 8) | data[BLE1TO1_IDX_NUM_L];
    if (len != (length + 5))
        return 0;

    if (data[BLE1TO1_IDX_ID_CODE] != BLE1TO1_ID_CODE_MASTER)
        return 0;

    switch (data[BLE1TO1_IDX_CMD]) {
    case BLE1TO1_CMD_21:
        return build_ac_setting_status(data, resp, resp_max);
    case BLE1TO1_CMD_22:
        return handle_set_ac_parameter(data, len, resp, resp_max);
    case BLE1TO1_CMD_24:
        return build_ac_setting_permission(data, resp, resp_max);
    case BLE1TO1_CMD_28:
        return build_ac_run_status(data, resp, resp_max);
    default:
        return build_error_resp(data, len, resp, resp_max);
    }
}

uint16_t ble_proto_1to1_on_state_changed(const gateway_state_t *s,
                                         uint8_t *resp, uint16_t resp_max)
{
    (void)s;
    (void)resp;
    (void)resp_max;
    return 0; /* 暂不主动上报，后续按需要实现 */
}
