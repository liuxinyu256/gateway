#ifndef THIRD_PARTY_MODULE_H
#define THIRD_PARTY_MODULE_H
#include "module.h"
#include "gateway_device.h"

/* 初始化参数 (通过 module_init 的 cfg 传入) */
typedef struct {
    uint32_t baudrate;
    timer_t *rx_timer;
    void (*write_byte)(uint8_t byte);
    gateway_device_t *gw;
} third_party_init_cfg_t;

extern const module_ops_t third_party_module_ops;

void third_party_module_init(module_t *m, gateway_device_t *gw);
void third_party_module_start(void);
#endif
