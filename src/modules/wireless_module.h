#ifndef WIRELESS_MODULE_H
#define WIRELESS_MODULE_H
#include "module.h"
#include "gateway_device.h"

typedef enum {
    WIRELESS_MIJIA_WIFI = 0x10,
    WIRELESS_MIJIA_BLE  = 0x11,
    WIRELESS_TUYA       = 0x12,
} wireless_brand_id_t;

typedef struct {
    const char            *name;
    const event_handler_t *evt_table;
} wireless_module_config_t;

/* 初始化参数 (通过 module_init 的 cfg 传入) */
typedef struct {
    uint32_t baudrate;
    timer_t *rx_timer;
    void (*write_byte)(uint8_t byte);
    gateway_device_t *gw;
} wireless_init_cfg_t;

extern const module_ops_t wireless_module_ops;

void wireless_module_init(module_t *m, gateway_device_t *gw);
void wireless_module_start(void);
#endif
