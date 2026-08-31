/**
 * board_hw.c —— 板级硬件抽象通用分发
 */
#include "board_hw.h"

uint8_t board_hw_init(board_hw_t *hw, const void *cfg)
{
    if (!hw || !hw->ops || !hw->ops->init)
        return 1;
    return hw->ops->init(hw, cfg);
}

void board_hw_rs485_enable(board_hw_t *hw, uint8_t enable)
{
    if (!hw || !hw->ops || !hw->ops->rs485_enable)
        return;
    hw->ops->rs485_enable(hw, enable);
}
