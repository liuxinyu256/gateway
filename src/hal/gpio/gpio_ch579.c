/**
 * gpio_ch579.c —— CH579 GPIO 驱动
 *
 * 支持 GPIOA / GPIOB 输出模式。
 * 当前仅实现输出（init/set/toggle），输入读取后续按需扩展。
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
    gpio_ch579_t      *self = (gpio_ch579_t *)g;
    const gpio_cfg_t  *c    = (const gpio_cfg_t *)cfg;

    if (!self || !c || !c->pin)
        return 1;

    self->port = c->port;
    self->pin  = c->pin;

    /* 平台无关模式 -> CH57x 库模式 */
    {
        GPIOModeTypeDef m;
        switch (c->mode) {
        case GPIO_MODE_INPUT:         m = GPIO_ModeIN_Floating; break;
        case GPIO_MODE_INPUT_PULLUP:  m = GPIO_ModeIN_PU;       break;
        case GPIO_MODE_INPUT_PULLDOWN: m = GPIO_ModeIN_PD;      break;
        case GPIO_MODE_OUTPUT_PP:     m = GPIO_ModeOut_PP_5mA;  break;
        case GPIO_MODE_OUTPUT_OD:
        default:
            return 1;   /* CH579 无开漏模式/未知模式 */
        }

        if (self->port == 1)
            GPIOB_ModeCfg(self->pin, m);
        else
            GPIOA_ModeCfg(self->pin, m);
    }

    ch579_set(g, c->init_level);
    return 0;
}

const gpio_ops_t gpio_ch579_ops = {
    .init   = ch579_init,
    .deinit = ch579_deinit,
    .set    = ch579_set,
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
