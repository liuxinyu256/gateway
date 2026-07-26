#ifndef WIFI_MGR_H
#define WIFI_MGR_H
#include "module.h"
#include "gateway_device.h"

void wifi_mgr_init(module_t *m, gateway_device_t *gw);
void wifi_mgr_start(void);
#endif
