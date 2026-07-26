#ifndef AC_BRAND_MANAGER_H
#define AC_BRAND_MANAGER_H
#include "module.h"
#include "brand.h"
#include "gateway_device.h"

#define MAX_BRANDS 8

typedef struct {
    uint8_t     type;       /* 编码: 电气类型+帧格式 */
    uint32_t    baudrate;
} brand_phy_t;

typedef struct {
    brand_id_t             brand_id;
    device_type_t          dev_type;
    const event_handler_t *evt_table;
    brand_phy_t            phy;
    uint16_t               receiver_timeout_ticks;
} brand_config_t;

void ac_brand_manager_init(module_t *m);
void ac_brand_manager_register(const brand_config_t *cfg, brand_t *ctx);
void ac_brand_manager_start_scan(void);
void ac_brand_manager_lock(void);
int  ac_brand_manager_locked(void);
const brand_config_t *ac_brand_manager_current(void);
void ac_brand_manager_set_poll_period(uint16_t period_ms);
#endif
