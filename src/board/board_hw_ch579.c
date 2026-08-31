/**
 * board_hw_ch579.c —— CH579 板级硬件实现
 *
 * 所有 GPIO 操作统一走 GPIO HAL，隔离具体引脚/电路差异。
 */
#include "board_hw_ch579.h"
#include "gpio_instance.h"

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

static uint8_t ch579_init(board_hw_t *hw, const void *cfg)
{
    board_hw_ch579_t *self = (board_hw_ch579_t *)hw;
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

static void ch579_rs485_enable(board_hw_t *hw, uint8_t enable)
{
    board_hw_ch579_t *self = (board_hw_ch579_t *)hw;
    if (!self)
        return;

    if (self->pb5)
        gpio_set(self->pb5, enable ? GPIO_LEVEL_HIGH : GPIO_LEVEL_LOW);
    if (self->pb6)
        gpio_set(self->pb6, enable ? GPIO_LEVEL_LOW : GPIO_LEVEL_HIGH);
}

const board_hw_ops_t board_hw_ch579_ops = {
    .init          = ch579_init,
    .rs485_enable  = ch579_rs485_enable,
};

uint8_t board_hw_ch579_init(board_hw_ch579_t *self,
                            const board_hw_ch579_cfg_t *cfg)
{
    if (!self)
        return 1;

    self->base.ops = &board_hw_ch579_ops;
    return board_hw_init(&self->base, cfg);
}
