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

void bsp_ac_select(bsp_t *hw, bsp_ac_brand_t brand)
{
    if (!hw || !hw->ops || !hw->ops->ac_select)
        return;   /* 该板不支持品牌切换，静默无效 */
    hw->ops->ac_select(hw, brand);
}

uint8_t bsp_capable(const bsp_t *hw, uint8_t cap)
{
    if (!hw)
        return 0;
    return (hw->caps & cap) ? 1 : 0;
}

const bsp_ac_phy_cfg_t *bsp_ac_phy_cfg(bsp_t *hw)
{
    if (!hw || !hw->ops || !hw->ops->get_ac_phy_cfg)
        return 0;
    return hw->ops->get_ac_phy_cfg(hw);
}
