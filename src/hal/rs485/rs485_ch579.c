/**
 * rs485_ch579.c —— CH579 RS485 方向控制驱动
 *
 * DE 引脚低电平=接收，高电平=发送。
 * 端口通过 cfg.port 选择：0=GPIOA，1=GPIOB。
 */
#include "rs485_ch579.h"

#ifdef __CH579__

static uint8_t ch579_init(rs485_t *rs, const void *cfg)
{
    rs485_ch579_t          *self = (rs485_ch579_t *)rs;
    const rs485_ch579_cfg_t *c   = (const rs485_ch579_cfg_t *)cfg;

    if (!self || !c || !c->de_pin)
        return 1;

    /* DE 引脚：推挽输出，默认低电平 = 接收方向 */
    gpio_cfg_t de_cfg = {
        .port       = c->port,
        .pin        = c->de_pin,
        .mode       = GPIO_MODE_OUT_PP_5MA,
        .init_level = 0,
    };
    return gpio_ch579_init(&self->de, &de_cfg);
}

static void ch579_set_dir(rs485_t *rs, uint8_t tx)
{
    rs485_ch579_t *self = (rs485_ch579_t *)rs;
    if (!self)
        return;

    gpio_set(&self->de.base, tx ? 1 : 0);
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
