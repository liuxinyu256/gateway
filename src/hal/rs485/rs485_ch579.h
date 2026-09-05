/**
 * rs485_ch579.h —— CH579 RS485 方向控制具体实现
 */
#ifndef RS485_CH579_H
#define RS485_CH579_H
#include "rs485.h"
#include "gpio_ch579.h"

typedef struct {
    rs485_t      base;
    gpio_ch579_t de;     /* DE 引脚 */
    gpio_ch579_t re;     /* RE 引脚（可选） */
    uint8_t      invert; /* 1=低电平发送/高电平接收 */
} rs485_ch579_t;

typedef struct {
    uint8_t  port;    /* 0=GPIOA, 1=GPIOB */
    uint32_t de_pin;  /* GPIO_Pin_x */
    uint32_t re_pin;  /* GPIO_Pin_x；0 表示不使用独立 RE */
    uint8_t  invert;  /* 1=低电平发送/高电平接收（部分硬件经过反相） */
} rs485_ch579_cfg_t;

extern const rs485_ops_t rs485_ch579_ops;

uint8_t rs485_ch579_init(rs485_ch579_t *self,
                         const rs485_ch579_cfg_t *cfg);

#endif /* RS485_CH579_H */
