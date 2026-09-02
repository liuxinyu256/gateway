/**
 * ble_proto_1to1.h —— 模块状态 <-> BLE 1to1 协议桥接（事件驱动）
 *
 * 只处理“当前模块”的状态：
 *   - 模块状态变化 -> ble_proto_1to1_on_state_changed() -> 打包上行帧
 *   - 上位机下发设置 -> ble_proto_1to1_on_rx() -> 修改模块状态/发控制命令
 */
#ifndef BLE_PROTO_1TO1_H
#define BLE_PROTO_1TO1_H

#include <stdint.h>
#include "gateway_device.h"

#define BLE1TO1_MODULE_ID        0   /* 当前只桥接 module 0 */

/* 帧位置 */
#define BLE1TO1_IDX_NUM_H        0
#define BLE1TO1_IDX_NUM_L        1
#define BLE1TO1_IDX_FLAG         2
#define BLE1TO1_IDX_DEV_TYPE     3
#define BLE1TO1_IDX_PARAM_TYPE   4
#define BLE1TO1_IDX_ID_CODE      5
#define BLE1TO1_IDX_CMD          6
#define BLE1TO1_IDX_DATA         7

#define BLE1TO1_ID_CODE_MASTER   0x5A
#define BLE1TO1_ID_CODE_GATEWAY  0xA5

/* 命令字：只实现状态相关 */
#define BLE1TO1_CMD_21  0x21  /* 查询模块状态 */
#define BLE1TO1_CMD_22  0x22  /* 设置模块状态 */

void ble_proto_1to1_init(void);

/* 上位机下发数据入口：返回需要回复的长度，0 表示无需回复 */
uint16_t ble_proto_1to1_on_rx(const uint8_t *data, uint16_t len,
                              uint8_t *resp, uint16_t resp_max);

/* 模块状态变化主动上报入口：返回需要通知的长度，0 表示无需通知 */
uint16_t ble_proto_1to1_on_state_changed(const gateway_state_t *s,
                                         uint8_t *resp, uint16_t resp_max);

#endif /* BLE_PROTO_1TO1_H */
