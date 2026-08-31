/**
 * rs485_ch579.c —— CH579 RS485 方向控制驱动
 *
 * DE 引脚低电平=接收，高电平=发送。
 * 当前实现仅支持 GPIOA（PA0~PA15）。
 */
#include "rs485_ch579.h"

#ifdef __CH579__
#include "CH57x_common.h"

static uint8_t ch579_init(rs485_t *rs, const void *cfg)
{
    rs485_ch579_t        *self = (rs485_ch579_t *)rs;
    const rs485_ch579_cfg_t *c = (const rs485_ch579_cfg_t *)cfg;

    if (!self || !c || !c->de_pin)
        return 1;

    self->de_pin = c->de_pin;

    /* 默认接收方向 */
    GPIOA_ResetBits(self->de_pin);
    GPIOA_ModeCfg(self->de_pin, GPIO_ModeOut_PP_5mA);
    return 0;
}

static void ch579_set_dir(rs485_t *rs, uint8_t tx)
{
    rs485_ch579_t *self = (rs485_ch579_t *)rs;
    if (!self)
        return;

    if (tx)
        GPIOA_SetBits(self->de_pin);
    else
        GPIOA_ResetBits(self->de_pin);
}

const rs485_ops_t rs485_ch579_ops = {
    .init    = ch579_init,
    .set_dir = ch579_set_dir,
};

uint8_t rs485_ch579_init(rs485_ch579_t *self,
                         const rs485_ch579_cfg_t *cfg)
{
    if (!self)
        return 1;

    self->base.ops = &rs485_ch579_ops;
    return rs485_init(&self->base, cfg);
}

#endif /* __CH579__ */
