/**
 * gpio_ch579.c —— CH579 GPIO 驱动
 *
 * 支持 GPIOA / GPIOB：
 *   - init    配置模式并设置初始电平
 *   - deinit  释放引脚（恢复高阻输入）
 *   - reset   恢复 init 时的模式和初始电平，不释放
 *   - set     输出电平
 *   - get     读取输入电平
 *   - toggle  翻转输出
 */
#include "gpio_ch579.h"

#ifdef __CH579__
#include "CH57x_common.h"

static void ch579_set(gpio_t *g, gpio_level_t level)
{
    gpio_ch579_t *self = (gpio_ch579_t *)g;
    if (!self)
        return;

    if (self->port == 1) {
        if (level == GPIO_LEVEL_HIGH)
            GPIOB_SetBits(self->pin);
        else
            GPIOB_ResetBits(self->pin);
    } else {
        if (level == GPIO_LEVEL_HIGH)
            GPIOA_SetBits(self->pin);
        else
            GPIOA_ResetBits(self->pin);
    }
}

static gpio_level_t ch579_get(gpio_t *g)
{
    gpio_ch579_t *self = (gpio_ch579_t *)g;
    uint8_t reg;

    if (!self || !self->pin)
        return GPIO_LEVEL_LOW;

    if (self->port == 1) {
        if (self->pin & 0xFF)
            reg = R8_PB_PIN_0;
        else
            reg = R8_PB_PIN_1;
    } else {
        if (self->pin & 0xFF)
            reg = R8_PA_PIN_0;
        else
            reg = R8_PA_PIN_1;
    }

    if (self->pin & 0xFF)
        return (reg & (uint8_t)self->pin) ? GPIO_LEVEL_HIGH : GPIO_LEVEL_LOW;
    return (reg & (uint8_t)(self->pin >> 8)) ? GPIO_LEVEL_HIGH : GPIO_LEVEL_LOW;
}

static void ch579_toggle(gpio_t *g)
{
    gpio_ch579_t *self = (gpio_ch579_t *)g;
    if (!self)
        return;

    if (self->port == 1)
        GPIOB_InverseBits(self->pin);
    else
        GPIOA_InverseBits(self->pin);
}

/* 平台无关模式 -> CH57x 库模式，并写入引脚配置寄存器 */
static uint8_t ch579_apply_mode(gpio_ch579_t *self, uint8_t mode)
{
    GPIOModeTypeDef m;

    switch (mode) {
    case GPIO_MODE_INPUT:          m = GPIO_ModeIN_Floating; break;
    case GPIO_MODE_INPUT_PULLUP:   m = GPIO_ModeIN_PU;       break;
    case GPIO_MODE_INPUT_PULLDOWN: m = GPIO_ModeIN_PD;       break;
    case GPIO_MODE_OUTPUT_PP:      m = GPIO_ModeOut_PP_5mA;  break;
    case GPIO_MODE_OUTPUT_OD:
    default:
        return 1;   /* CH579 无开漏模式/未知模式 */
    }

    if (self->port == 1)
        GPIOB_ModeCfg(self->pin, m);
    else
        GPIOA_ModeCfg(self->pin, m);
    return 0;
}

static void ch579_reset(gpio_t *g)
{
    gpio_ch579_t *self = (gpio_ch579_t *)g;
    if (!self || !self->pin)
        return;

    if (ch579_apply_mode(self, self->mode) != 0)
        return;
    ch579_set(g, self->init_level);
}

static void ch579_deinit(gpio_t *g)
{
    gpio_ch579_t *self = (gpio_ch579_t *)g;
    if (!self || !self->pin)
        return;

    /* 释放引脚：恢复高阻输入，避免影响外部电路 */
    if (self->port == 1)
        GPIOB_ModeCfg(self->pin, GPIO_ModeIN_Floating);
    else
        GPIOA_ModeCfg(self->pin, GPIO_ModeIN_Floating);
}

static uint8_t ch579_init(gpio_t *g, const void *cfg)
{
    gpio_ch579_t     *self = (gpio_ch579_t *)g;
    const gpio_cfg_t *c    = (const gpio_cfg_t *)cfg;

    if (!self || !c || !c->pin)
        return 1;

    self->port      = c->port;
    self->pin       = c->pin;
    self->mode      = c->mode;
    self->init_level = c->init_level;

    if (ch579_apply_mode(self, self->mode) != 0)
        return 1;

    ch579_set(g, self->init_level);
    return 0;
}

const gpio_ops_t gpio_ch579_ops = {
    .init   = ch579_init,
    .deinit = ch579_deinit,
    .reset  = ch579_reset,
    .set    = ch579_set,
    .get    = ch579_get,
    .toggle = ch579_toggle,
};

uint8_t gpio_ch579_init(gpio_ch579_t *self, const gpio_cfg_t *cfg)
{
    if (!self)
        return 1;

    self->base.ops = &gpio_ch579_ops;
    return gpio_init(&self->base, cfg);
}

#endif /* __CH579__ */
