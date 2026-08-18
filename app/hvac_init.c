/**
 * hvac_init.c —— 网关初始化
 */

#include "hvac_init.h"
#include "gateway.h"
#include "ac_module.h"
#include "frame_timer_hw.h"
#include "FreeRTOS.h"

extern const brand_config_t gree_brand;

static ac_brand_t g_gree_ctx;
static module_t   g_module_hvac;
static ac_module_t g_ac;

void hvac_start(void) {
    gateway_init();

    /* hardware init deferred to brand on_activate
     *    RS485, UART1, 9600bps, tx=9, rx=8, de=0 */
    module_init(&g_module_hvac, 9600, RING_HVAC_AC,
                frame_timer_hw_create(0), 5);
    gateway_set_module(0, &g_module_hvac);

    ac_module_init(&g_ac, &g_module_hvac);
    ac_module_register(&g_ac, &gree_brand, &g_gree_ctx);

    module_start(&g_module_hvac);
    ac_module_start_scan(&g_ac);
}
