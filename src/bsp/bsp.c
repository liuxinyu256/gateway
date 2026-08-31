/**
 * bsp.c —— 板级硬件抽象通用分发
 */
#include "bsp.h"

uint8_t bsp_init(bsp_t *hw, const void *cfg)
{
    if (!hw || !hw->ops || !hw->ops->init)
        return 1;
    return hw->ops->init(hw, cfg);
}

void bsp_rs485_enable(bsp_t *hw, uint8_t enable)
{
    if (!hw || !hw->ops || !hw->ops->rs485_enable)
        return;
    hw->ops->rs485_enable(hw, enable);
}
