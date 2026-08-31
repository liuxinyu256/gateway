/**
 * bsp_ch579.c —— CH579 板级硬件实现
 *
 * 所有 GPIO 操作统一走 GPIO HAL，隔离具体引脚/电路差异。
 */
#include "bsp_ch579.h"
#include "gpio_instance.h"
#include <stddef.h>

static void cfg_pin(gpio_t **slot, uint8_t port, uint8_t pin,
                    uint8_t mode, gpio_level_t level)
{
    gpio_t *g = gpio_get(port, pin);
    gpio_cfg_t cfg;

    if (!g)
        return;

    cfg.port       = port;
    cfg.pin        = (uint32_t)(1u << pin);
    cfg.mode       = mode;
    cfg.init_level = level;
    gpio_init(g, &cfg);

    if (slot)
        *slot = g;
}

static uint8_t ch579_init(bsp_t *hw, const void *cfg)
{
    bsp_ch579_t *self = (bsp_ch579_t *)hw;
    (void)cfg;

    if (!self)
        return 1;

    /* 关闭海尔多联机通讯电路 */
    cfg_pin(&self->pb8, GPIO_PORT_B, 8,  GPIO_MODE_OUTPUT_PP, GPIO_LEVEL_LOW);

    /* PB11/PB21 浮空输入 */
    cfg_pin(&self->pb11, GPIO_PORT_B, 11, GPIO_MODE_INPUT, GPIO_LEVEL_LOW);
    cfg_pin(&self->pb21, GPIO_PORT_B, 21, GPIO_MODE_INPUT, GPIO_LEVEL_LOW);

    /* 关闭 120 电阻 */
    cfg_pin(&self->pb1, GPIO_PORT_B, 1,  GPIO_MODE_OUTPUT_PP, GPIO_LEVEL_LOW);

    /* PA14 浮空输入 */
    cfg_pin(&self->pa14, GPIO_PORT_A, 14, GPIO_MODE_INPUT, GPIO_LEVEL_LOW);

    /* 关掉东芝电路，电源先选择美的 */
    cfg_pin(&self->pb9, GPIO_PORT_B, 9,  GPIO_MODE_OUTPUT_PP, GPIO_LEVEL_HIGH);

    /* PA15 浮空输入，接收口选择美的 */
    cfg_pin(&self->pa15, GPIO_PORT_A, 15, GPIO_MODE_INPUT, GPIO_LEVEL_LOW);

    /* 打开 485 电路 */
    cfg_pin(&self->pb6, GPIO_PORT_B, 6,  GPIO_MODE_OUTPUT_PP, GPIO_LEVEL_LOW);
    cfg_pin(&self->pb5, GPIO_PORT_B, 5,  GPIO_MODE_OUTPUT_PP, GPIO_LEVEL_HIGH);

    return 0;
}

static void ch579_rs485_enable(bsp_t *hw, uint8_t enable)
{
    bsp_ch579_t *self = (bsp_ch579_t *)hw;
    if (!self)
        return;

    if (self->pb5)
        gpio_set(self->pb5, enable ? GPIO_LEVEL_HIGH : GPIO_LEVEL_LOW);
    if (self->pb6)
        gpio_set(self->pb6, enable ? GPIO_LEVEL_LOW : GPIO_LEVEL_HIGH);
}

const bsp_ops_t bsp_ch579_ops = {
    .init          = ch579_init,
    .rs485_enable  = ch579_rs485_enable,
};

uint8_t bsp_ch579_init(bsp_ch579_t *self,
                       const bsp_ch579_cfg_t *cfg)
{
    if (!self)
        return 1;

    self->base.ops = &bsp_ch579_ops;
    return bsp_init(&self->base, cfg);
}

/* ---- 板级选择接口 ---- */
static bsp_ch579_t g_bsp;

uint8_t bsp_ch579_board_init(void)
{
    return bsp_ch579_init(&g_bsp, NULL);
}

bsp_t *bsp_ch579_board_get(void)
{
    return &g_bsp.base;
}
