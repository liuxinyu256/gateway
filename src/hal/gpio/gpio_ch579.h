/**
 * gpio_ch579.h —— CH579 GPIO 具体实现
 */
#ifndef GPIO_CH579_H
#define GPIO_CH579_H
#include "gpio.h"

typedef struct {
    gpio_t   base;
    uint8_t  port;  /* 0=GPIOA, 1=GPIOB */
    uint32_t pin;   /* GPIO_Pin_x */
} gpio_ch579_t;

extern const gpio_ops_t gpio_ch579_ops;

uint8_t gpio_ch579_init(gpio_ch579_t *self, const gpio_cfg_t *cfg);

#endif /* GPIO_CH579_H */
