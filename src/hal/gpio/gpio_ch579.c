/**
 * gpio_ch579.c —— CH579 GPIO 驱动
 *
 * 支持 GPIOA / GPIOB 输出模式。
 * 当前仅实现输出（init/set/toggle），输入读取后续按需扩展。
 */
#include "gpio_ch579.h"

#ifdef __CH579__
#include "CH57x_common.h"

static void ch579_set(gpio_t *g, uint8_t level)
{
    gpio_ch579_t *self = (gpio_ch579_t *)g;
    if (!self)
        return;

    if (self->port == 1) {
        if (level)
            GPIOB_SetBits(self->pin);
        else
            GPIOB_ResetBits(self->pin);
    } else {
        if (level)
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

static uint8_t ch579_init(gpio_t *g, const void *cfg)
{
    gpio_ch579_t      *self = (gpio_ch579_t *)g;
    const gpio_cfg_t  *c    = (const gpio_cfg_t *)cfg;

    if (!self || !c || !c->pin)
        return 1;

    self->port = c->port;
    self->pin  = c->pin;

    /* 按平台无关模式映射到 CH57x GPIOModeTypeDef */
    {
        GPIOModeTypeDef m;
        switch (c->mode) {
        case GPIO_MODE_IN_FLOATING: m = GPIO_ModeIN_Floating; break;
        case GPIO_MODE_IN_PU:       m = GPIO_ModeIN_PU;       break;
        case GPIO_MODE_IN_PD:       m = GPIO_ModeIN_PD;       break;
        case GPIO_MODE_OUT_PP_20MA: m = GPIO_ModeOut_PP_20mA; break;
        case GPIO_MODE_OUT_PP_5MA:
        default:                    m = GPIO_ModeOut_PP_5mA;  break;
        }

        if (self->port == 1)
            GPIOB_ModeCfg(self->pin, m);
        else
            GPIOA_ModeCfg(self->pin, m);
    }

    ch579_set(g, c->init_level ? 1 : 0);
    return 0;
}

const gpio_ops_t gpio_ch579_ops = {
    .init   = ch579_init,
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
