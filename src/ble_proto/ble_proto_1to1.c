/**
 * ble_proto_1to1.c —— 模块状态 <-> BLE 1to1 协议桥接
 *
 * 只处理 module 0 的 gateway_state_t：
 *   - 0x21 查询模块状态
 *   - 0x22 设置模块状态
 * 帧格式按协议文档示例：
 *   [len_hi,len_lo,flag,dev_type,param_type,id_code,cmd,data...]
 *   len = 总长度 - 5
 */
#include "ble_proto_1to1.h"
#include "gateway.h"
#include "module.h"
#include <string.h>

#define BLE1TO1_STATE_LEN  10

void ble_proto_1to1_init(void)
{
}

/* 将 gateway_state_t 打包成 10 字节状态块 */
static void pack_state(const gateway_state_t *s, uint8_t out[BLE1TO1_STATE_LEN])
{
    memset(out, 0, BLE1TO1_STATE_LEN);
    out[0] = 0x01;              /* 通讯正常 */
    out[1] = 0x01;              /* 在线 */
    out[2] = s->power;
    out[3] = s->mode;
    out[4] = s->fan;
    out[5] = s->room_temp;
    out[6] = s->set_temp;
    out[7] = s->swing;
    out[8] = s->error_code;
    out[9] = 0x00;              /* 保留 */
}

/* 将 10 字节状态块解包到 gateway_state_t（只取本模块关心的字段） */
static void unpack_state(const uint8_t in[BLE1TO1_STATE_LEN], gateway_state_t *s)
{
    s->power      = in[2];
    s->mode       = in[3];
    s->fan        = in[4];
    s->room_temp  = in[5];
    s->set_temp   = in[6];
    s->swing      = in[7];
    s->error_code = in[8];
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

/* 0x21：查询模块状态 */
static uint16_t handle_query_state(const uint8_t *rx, uint8_t dev_type,
                                   uint8_t *resp, uint16_t resp_max)
{
    gateway_state_t s;
    uint8_t st[BLE1TO1_STATE_LEN];
    uint8_t data[2 + BLE1TO1_STATE_LEN];

    if (gateway_module_state_get(BLE1TO1_MODULE_ID, &s) != 0)
        memset(&s, 0, sizeof(s));

    pack_state(&s, st);
    data[0] = 0xFF;  /* AC Address 默认 0xFFFF */
    data[1] = 0xFF;
    memcpy(&data[2], st, BLE1TO1_STATE_LEN);

    /* 0x21 响应 param_type 示例为 0xF0 */
    return build_frame(BLE1TO1_CMD_21, 0xF0, data, sizeof(data),
                       resp, resp_max, dev_type);
}

/* 0x22：设置模块状态 */
static uint16_t handle_set_state(const uint8_t *rx, uint16_t rx_len,
                                 uint8_t dev_type,
                                 uint8_t *resp, uint16_t resp_max)
{
    const uint8_t *p = &rx[BLE1TO1_IDX_DATA];
    uint16_t remain = rx_len - BLE1TO1_IDX_DATA;
    uint8_t st[BLE1TO1_STATE_LEN];
    gateway_state_t s;
    uint8_t result = 0x00;
    uint8_t data[1];

    /* 请求数据：2字节地址 + 状态块（或只给状态块，兼容处理） */
    if (remain >= 2 + BLE1TO1_STATE_LEN)
        memcpy(st, &p[2], BLE1TO1_STATE_LEN);
    else if (remain >= BLE1TO1_STATE_LEN)
        memcpy(st, p, BLE1TO1_STATE_LEN);
    else
        result = 0x01; /* 数据解析错误 */

    if (result == 0x00) {
        if (gateway_module_state_get(BLE1TO1_MODULE_ID, &s) != 0)
            memset(&s, 0, sizeof(s));
        unpack_state(st, &s);
        module_update_state(gateway_module(BLE1TO1_MODULE_ID), &s);
    }

    data[0] = result;
    /* 0x22 响应 param_type 示例为 0xC7 */
    return build_frame(BLE1TO1_CMD_22, 0xC7, data, sizeof(data),
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
        return 0; /* 其他命令暂不处理 */
    }
}

uint16_t ble_proto_1to1_on_state_changed(const gateway_state_t *s,
                                         uint8_t *resp, uint16_t resp_max)
{
    uint8_t st[BLE1TO1_STATE_LEN];
    uint8_t data[2 + BLE1TO1_STATE_LEN];

    if (!s || !resp || resp_max < 19)
        return 0;

    pack_state(s, st);
    data[0] = 0xFF;
    data[1] = 0xFF;
    memcpy(&data[2], st, BLE1TO1_STATE_LEN);

    return build_frame(BLE1TO1_CMD_21, 0xF0, data, sizeof(data),
                       resp, resp_max, 0x2D);
}
