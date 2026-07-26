/**
 * hvac_init.c —— 网关初始化
 */

#include "hvac_init.h"
#include "gateway.h"
#include "ac_brand_manager.h"
#include "frame_timer_hw.h"
#include "FreeRTOS.h"

extern const brand_config_t gree_brand;

static brand_t   g_gree_ctx;
static module_t  g_module_hvac;

void hvac_start(void) {
    gateway_init();

    phy_config_t phy_cfg = {
        .type     = PHY_RS485_8N1,
        .uart_id  = 1,
        .baudrate = 9600,
        .tx_pin   = 9, .rx_pin = 8, .de_pin = 0,
    };
    phy_driver_t *phy = phy_create(&phy_cfg);

    module_init(&g_module_hvac, 0, phy, 9600, RING_HVAC_AC,
                frame_timer_hw_create(0), 5);
    gateway_set_module(0, &g_module_hvac);

    ac_brand_manager_init(&g_module_hvac);
    ac_brand_manager_register(&gree_brand, &g_gree_ctx);

    module_start(&g_module_hvac);
    ac_brand_manager_start_scan();
}
