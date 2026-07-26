/**
 * phy_factory.c —— 物理层统一工厂
 * 填 phy_config_t → phy_create(dispatch) → phy_driver_t*
 */

#include "phy.h"
#include "phy_uart1.h"

static phy_driver_t *phy_create_uart_hw(const phy_config_t *cfg) {
    switch (cfg->uart_id) {
    case 1: return phy_uart1_create(NULL);
    default: return NULL;
    }
}

phy_driver_t *phy_create(const phy_config_t *cfg) {
    if (!cfg) return NULL;
    /* 高4位取电气类型 */
    switch (cfg->type >> 4) {
    case 1: /* RS485 */
    case 2: /* TTL */
    case 3: /* HBS */
        return phy_create_uart_hw(cfg);
    case 4: /* SW_UART */
    default:
        return NULL;
    }
}
