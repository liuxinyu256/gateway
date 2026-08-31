/**
 * rs485_ch579.h —— CH579 RS485 方向控制具体实现
 */
#ifndef RS485_CH579_H
#define RS485_CH579_H
#include "rs485.h"
#include "gpio_ch579.h"

typedef struct {
    rs485_t      base;
    gpio_ch579_t de;   /* DE 引脚由 GPIO HAL 驱动 */
} rs485_ch579_t;

typedef struct {
    uint8_t  port;   /* 0=GPIOA, 1=GPIOB */
    uint32_t de_pin; /* GPIO_Pin_x */
} rs485_ch579_cfg_t;

extern const rs485_ops_t rs485_ch579_ops;

uint8_t rs485_ch579_init(rs485_ch579_t *self,
                         const rs485_ch579_cfg_t *cfg);

#endif /* RS485_CH579_H */
