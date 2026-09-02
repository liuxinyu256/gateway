/**
 * ble_module.h —— BLE 模块骨架
 */
#ifndef BLE_MODULE_H
#define BLE_MODULE_H
#include "module.h"
#include "gateway_device.h"

typedef struct {
    module_t base;
    uint8_t  connected;
    uint8_t  bonded;
} ble_module_t;

extern const module_ops_t ble_module_ops;

void ble_module_start(void);

#endif /* BLE_MODULE_H */
