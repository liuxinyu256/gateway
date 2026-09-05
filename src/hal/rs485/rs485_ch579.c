/**
 * rs485_ch579.c —— CH579 RS485 方向控制驱动
 *
 * DE 引脚低电平=接收，高电平=发送。
 * RE 引脚低电平=接收使能，高电平=发送期间关闭接收（可选）。
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

    self->invert = c->invert ? 1 : 0;

    /* DE 引脚：默认接收方向；invert=1 时接收为高，发送为低 */
    gpio_cfg_t de_cfg = {
        .port       = c->port,
        .pin        = c->de_pin,
        .mode       = GPIO_MODE_OUTPUT_PP,
        .init_level = c->invert ? GPIO_LEVEL_HIGH : GPIO_LEVEL_LOW,
    };
    if (gpio_ch579_init(&self->de, &de_cfg) != 0)
        return 1;

    /* RE 引脚：推挽输出，默认低电平 = 接收使能 */
    if (c->re_pin) {
        gpio_cfg_t re_cfg = {
            .port       = c->port,
            .pin        = c->re_pin,
            .mode       = GPIO_MODE_OUTPUT_PP,
            .init_level = GPIO_LEVEL_LOW,
        };
        if (gpio_ch579_init(&self->re, &re_cfg) != 0)
            return 1;
    }

    return 0;
}

static void ch579_set_dir(rs485_t *rs, uint8_t tx)
{
    rs485_ch579_t *self = (rs485_ch579_t *)rs;
    gpio_level_t de_level;

    if (!self)
        return;

    if (self->invert)
        de_level = tx ? GPIO_LEVEL_LOW : GPIO_LEVEL_HIGH;
    else
        de_level = tx ? GPIO_LEVEL_HIGH : GPIO_LEVEL_LOW;

    gpio_set(&self->de.base, de_level);

    if (self->re.base.ops) {
        if (self->invert) {
            /* 反相硬件暂不使用独立 RE，保持接收使能 */
            gpio_set(&self->re.base, GPIO_LEVEL_LOW);
        } else {
            /* AC 口：发送 RE 高（关接收），接收 RE 低（开接收） */
            gpio_set(&self->re.base,
                     tx ? GPIO_LEVEL_HIGH : GPIO_LEVEL_LOW);
        }
    }
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
