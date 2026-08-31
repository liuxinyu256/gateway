/**
 * bsp_a07s.c —— A07S 板级硬件实现
 *
 * A07S 不使用美的/东芝/海尔品牌选择电路，
 * 但用不到的电路引脚必须显式配置为关闭/禁用状态。
 * 所有 GPIO 操作统一走 GPIO HAL。
 */
#include "bsp_a07s.h"
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
    bsp_a07s_t *self = (bsp_a07s_t *)hw;
    (void)cfg;

    if (!self)
        return 1;

    /* 声明本板能力：只支持 485，无品牌电路切换 */
    hw->caps = BSP_CAP_RS485;

    /* 用不到的海尔多联机通讯电路：关闭 */
    cfg_pin(&self->pb8, GPIO_PORT_B, 8,  GPIO_MODE_OUTPUT_PP, GPIO_LEVEL_LOW);

    /* 用不到的 PB11/PB21：浮空输入（禁用） */
    cfg_pin(&self->pb11, GPIO_PORT_B, 11, GPIO_MODE_INPUT, GPIO_LEVEL_LOW);
    cfg_pin(&self->pb21, GPIO_PORT_B, 21, GPIO_MODE_INPUT, GPIO_LEVEL_LOW);

    /* 用不到的 120 电阻：关闭 */
    cfg_pin(&self->pb1, GPIO_PORT_B, 1,  GPIO_MODE_OUTPUT_PP, GPIO_LEVEL_LOW);

    /* 用不到的 PA14：浮空输入（禁用） */
    cfg_pin(&self->pa14, GPIO_PORT_A, 14, GPIO_MODE_INPUT, GPIO_LEVEL_LOW);

    /* 用不到的东芝/美的电源选择：关闭东芝 */
    cfg_pin(&self->pb9, GPIO_PORT_B, 9,  GPIO_MODE_OUTPUT_PP, GPIO_LEVEL_HIGH);

    /* 用不到的 PA15：浮空输入（禁用） */
    cfg_pin(&self->pa15, GPIO_PORT_A, 15, GPIO_MODE_INPUT, GPIO_LEVEL_LOW);

    /* 打开 485 电路 */
    cfg_pin(&self->pb6, GPIO_PORT_B, 6,  GPIO_MODE_OUTPUT_PP, GPIO_LEVEL_LOW);
    cfg_pin(&self->pb5, GPIO_PORT_B, 5,  GPIO_MODE_OUTPUT_PP, GPIO_LEVEL_HIGH);

    return 0;
}

static void ch579_rs485_enable(bsp_t *hw, uint8_t enable)
{
    bsp_a07s_t *self = (bsp_a07s_t *)hw;
    if (!self)
        return;

    if (self->pb5)
        gpio_set(self->pb5, enable ? GPIO_LEVEL_HIGH : GPIO_LEVEL_LOW);
    if (self->pb6)
        gpio_set(self->pb6, enable ? GPIO_LEVEL_LOW : GPIO_LEVEL_HIGH);
}

const bsp_ops_t bsp_a07s_ops = {
    .init          = ch579_init,
    .rs485_enable  = ch579_rs485_enable,
    .ac_select     = NULL,   /* A07S 无品牌电路切换 */
};

uint8_t bsp_a07s_init(bsp_a07s_t *self,
                       const bsp_a07s_cfg_t *cfg)
{
    if (!self)
        return 1;

    self->base.ops = &bsp_a07s_ops;
    return bsp_init(&self->base, cfg);
}

/* ---- 板级选择接口 ---- */
static bsp_a07s_t g_bsp;

uint8_t bsp_a07s_board_init(void)
{
    return bsp_a07s_init(&g_bsp, NULL);
}

bsp_t *bsp_a07s_board_get(void)
{
    return &g_bsp.base;
}
