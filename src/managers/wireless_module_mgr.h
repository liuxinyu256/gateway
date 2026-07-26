#ifndef WIRELESS_MODULE_MGR_H
#define WIRELESS_MODULE_MGR_H
#include "module.h"
#include "gateway_device.h"

typedef struct {
    const char            *name;
    const event_handler_t *evt_table;
} wireless_module_config_t;

void wireless_module_mgr_init(module_t *m, gateway_device_t *gw);
void wireless_module_mgr_start(void);
#endif
