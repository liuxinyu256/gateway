/**
 * ble_proto_1to1.c —— 模块状态 <-> 旧项目 1to1 BLE 协议
 *
 * 按旧项目 proBLEComm1to1_v1d0 的一控一空调帧格式：
 *   0x21 查询：响应 15 字节
 *     data[0..1]=通讯/在线
 *     data[2]=power data[3]=mode data[4]=wind/fan
 *     data[5]=room_temp data[6]=set_temp data[7]=master/slave
 *   0x22 设置：请求 data[0..4]=power/mode/wind/set_temp/master
 */
#include "ble_proto_1to1.h"
#include "gateway.h"
#include "module.h"
#include <string.h>

#define BLE1TO1_QUERY_DATA_LEN  8   /* 0x21 响应数据长度 */
#define BLE1TO1_SET_DATA_LEN    5   /* 0x22 请求数据长度 */

void ble_proto_1to1_init(void)
{
}

static void pack_state(const gateway_state_t *s, uint8_t out[BLE1TO1_QUERY_DATA_LEN])
{
    memset(out, 0, BLE1TO1_QUERY_DATA_LEN);
    out[0] = 0x01;              /* 通讯正常 */
    out[1] = 0x01;              /* 在线 */
    out[2] = s->power;
    out[3] = s->mode;
    out[4] = s->fan;
    out[5] = s->room_temp;
    out[6] = s->set_temp;
    out[7] = 0x00;              /* master/slave */
}

static void unpack_set_data(const uint8_t in[BLE1TO1_SET_DATA_LEN], gateway_state_t *s)
{
    s->power    = in[0];
    s->mode     = in[1];
    s->fan      = in[2];
    s->set_temp = in[3];
    /* in[4] = master/slave，暂不处理 */
}

static uint16_t build_frame(uint8_t cmd, uint8_t param, const uint8_t *data,
                            uint16_t data_len, uint8_t *resp, uint16_t resp_max,
                            uint8_t dev_type)
{
    uint16_t total = 7 + data_len;

    if (resp_max < total)
        return 0;

    memset(resp, 0, total);
    resp[BLE1TO1_IDX_NUM_H] = (uint8_t)((total - 5) >> 8);
    resp[BLE1TO1_IDX_NUM_L] = (uint8_t)((total - 5) & 0xFF);
    resp[BLE1TO1_IDX_FLAG]       = 0x00;
    resp[BLE1TO1_IDX_DEV_TYPE]   = dev_type;
    resp[BLE1TO1_IDX_PARAM_TYPE] = param;
    resp[BLE1TO1_IDX_ID_CODE]    = BLE1TO1_ID_CODE_GATEWAY;
    resp[BLE1TO1_IDX_CMD]        = cmd;

    if (data_len && data)
        memcpy(&resp[BLE1TO1_IDX_DATA], data, data_len);

    return total;
}

static uint16_t handle_query_state(const uint8_t *rx, uint8_t dev_type,
                                   uint8_t *resp, uint16_t resp_max)
{
    gateway_state_t s;
    uint8_t data[BLE1TO1_QUERY_DATA_LEN];

    if (gateway_module_state_get(BLE1TO1_MODULE_ID, &s) != 0)
        memset(&s, 0, sizeof(s));

    pack_state(&s, data);
    /* 旧项目 0x21 响应 param_type=0x11, id=0xA5 */
    return build_frame(BLE1TO1_CMD_21, 0x11, data, sizeof(data),
                       resp, resp_max, dev_type);
}

static uint16_t handle_set_state(const uint8_t *rx, uint16_t rx_len,
                                 uint8_t dev_type,
                                 uint8_t *resp, uint16_t resp_max)
{
    const uint8_t *p = &rx[BLE1TO1_IDX_DATA];
    uint16_t remain = rx_len - BLE1TO1_IDX_DATA;
    gateway_state_t s;
    uint8_t result = 0x00;
    uint8_t data[1];

    if (remain < BLE1TO1_SET_DATA_LEN) {
        result = 0x01;
    } else {
        if (gateway_module_state_get(BLE1TO1_MODULE_ID, &s) != 0)
            memset(&s, 0, sizeof(s));
        unpack_set_data(p, &s);
        module_update_state(gateway_module(BLE1TO1_MODULE_ID), &s);
    }

    data[0] = result;
    /* 旧项目 0x22 响应 param_type=0x21 */
    return build_frame(BLE1TO1_CMD_22, 0x21, data, sizeof(data),
                       resp, resp_max, dev_type);
}

uint16_t ble_proto_1to1_on_rx(const uint8_t *data, uint16_t len,
                              uint8_t *resp, uint16_t resp_max)
{
    uint16_t length;
    uint8_t dev_type;

    if (!data || len < 8 || !resp || resp_max < 8)
        return 0;

    length = ((uint16_t)data[BLE1TO1_IDX_NUM_H] << 8) | data[BLE1TO1_IDX_NUM_L];
    if (len != (length + 5))
        return 0;

    if (data[BLE1TO1_IDX_ID_CODE] != BLE1TO1_ID_CODE_MASTER)
        return 0;

    dev_type = data[BLE1TO1_IDX_DEV_TYPE];

    switch (data[BLE1TO1_IDX_CMD]) {
    case BLE1TO1_CMD_21:
        return handle_query_state(data, dev_type, resp, resp_max);
    case BLE1TO1_CMD_22:
        return handle_set_state(data, len, dev_type, resp, resp_max);
    default:
        return 0;
    }
}

uint16_t ble_proto_1to1_on_state_changed(const gateway_state_t *s,
                                         uint8_t *resp, uint16_t resp_max)
{
    uint8_t data[BLE1TO1_QUERY_DATA_LEN];

    if (!s || !resp || resp_max < (7 + BLE1TO1_QUERY_DATA_LEN))
        return 0;

    pack_state(s, data);
    return build_frame(BLE1TO1_CMD_21, 0x11, data, sizeof(data),
                       resp, resp_max, 0x2D);
}
