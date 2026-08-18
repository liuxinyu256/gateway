#ifndef WIFI_MODULE_H
#define WIFI_MODULE_H
#include "module.h"
#include "gateway_device.h"

typedef enum {
    WIFI_ESPRESSIF = 0x20,
    WIFI_REALTEK   = 0x21,
} wifi_chip_id_t;

/* 初始化参数 (通过 module_init 的 cfg 传入) */
typedef struct {
    uint32_t baudrate;
    timer_t *rx_timer;
    void (*write_byte)(uint8_t byte);
    gateway_device_t *gw;
} wifi_init_cfg_t;

extern const module_ops_t wifi_module_ops;

void wifi_module_init(module_t *m, gateway_device_t *gw);
void wifi_module_start(void);
#endif
