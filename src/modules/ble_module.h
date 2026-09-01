/**
 * ble_module.h —— BLE 模块
 *
 * 复用 module_t 框架接入 CH57x BLE 协议栈。
 * 当前为骨架，后续在 ble_phy 里接入 CH57xBLE.lib + TMOS。
 */
#ifndef BLE_MODULE_H
#define BLE_MODULE_H
#include "module.h"
#include "gateway_device.h"

typedef struct {
    module_t base;
    uint8_t  connected;   /* 1=已连接 */
    uint8_t  bonded;      /* 1=已配对 */
} ble_module_t;

extern const module_ops_t ble_module_ops;

void ble_module_start(void);

#endif /* BLE_MODULE_H */
