/**
 * ble_proto_1to1.h —— 1to1 BLE 协议（事件驱动版）
 *
 * 对接方式：
 *   - 手机写 CHAR1 -> ble_proto_1to1_on_rx()
 *   - 返回数据 -> 通过 CHAR4 通知发回手机
 *   - AC 状态变化 -> ble_proto_1to1_on_state_changed()
 */
#ifndef BLE_PROTO_1TO1_H
#define BLE_PROTO_1TO1_H

#include <stdint.h>
#include "gateway_device.h"

/* 协议帧位置 */
#define BLE1TO1_IDX_NUM_H        0
#define BLE1TO1_IDX_NUM_L        1
#define BLE1TO1_IDX_FLAG         2
#define BLE1TO1_IDX_DEV_TYPE     3
#define BLE1TO1_IDX_PARAM_TYPE   4
#define BLE1TO1_IDX_ID_CODE      5
#define BLE1TO1_IDX_CMD          6
#define BLE1TO1_IDX_DATA         7

#define BLE1TO1_ID_CODE_MASTER   0xA5
#define BLE1TO1_DEV_TYPE_FA      0xFA

/* 命令字 */
#define BLE1TO1_CMD_00  0x00  /* 读取开关/基本信息 */
#define BLE1TO1_CMD_0E  0x0E  /* 读取网关485地址 */
#define BLE1TO1_CMD_0F  0x0F  /* 设置网关485地址 */
#define BLE1TO1_CMD_10  0x10  /* 查询网关无线模块状态 */
#define BLE1TO1_CMD_11  0x11  /* 设置网关无线模块状态 */
#define BLE1TO1_CMD_18  0x18  /* 复位 */
#define BLE1TO1_CMD_1F  0x1F  /* 读取小程序信息 */
#define BLE1TO1_CMD_21  0x21  /* 查询空调设定状态 */
#define BLE1TO1_CMD_22  0x22  /* 设置空调设定参数 */
#define BLE1TO1_CMD_24  0x24  /* 读取空调设定权限 */
#define BLE1TO1_CMD_26  0x26  /* 空调品牌切换 */
#define BLE1TO1_CMD_28  0x28  /* 查询空调运行/故障信息 */
#define BLE1TO1_CMD_29  0x29  /* 查询新风设定状态 */
#define BLE1TO1_CMD_2A  0x2A  /* 设置新风设定状态 */
#define BLE1TO1_CMD_2B  0x2B  /* 查询地暖设定状态 */
#define BLE1TO1_CMD_2C  0x2C  /* 设置地暖设定状态 */
#define BLE1TO1_CMD_91  0x91  /* 开始抓包 */
#define BLE1TO1_CMD_92  0x92  /* 上传抓包数据 */
#define BLE1TO1_CMD_93  0x93  /* 停止抓包 */
#define BLE1TO1_CMD_94  0x94  /* 开始透传 */
#define BLE1TO1_CMD_95  0x95  /* 下发485数据 */
#define BLE1TO1_CMD_96  0x96  /* 下发空调数据 */
#define BLE1TO1_CMD_97  0x97  /* 停止透传 */
#define BLE1TO1_CMD_A0  0xA0  /* 透传数据到TTL */
#define BLE1TO1_CMD_A1  0xA1  /* 透传TTL数据 */

void ble_proto_1to1_init(void);

/* BLE 收到手机数据：返回需要回复的数据长度，0 表示无需回复 */
uint16_t ble_proto_1to1_on_rx(const uint8_t *data, uint16_t len,
                              uint8_t *resp, uint16_t resp_max);

/* AC/网关状态变化时主动上报：返回需要上报的数据长度，0 表示无需上报 */
uint16_t ble_proto_1to1_on_state_changed(const gateway_state_t *s,
                                         uint8_t *resp, uint16_t resp_max);

#endif /* BLE_PROTO_1TO1_H */
