/**
 * gpio_ch579.h —— CH579 GPIO 具体实现
 */
#ifndef GPIO_CH579_H
#define GPIO_CH579_H
#include "gpio.h"

typedef struct {
    gpio_t base;   /* CH579 暂无额外平台私有数据 */
} gpio_ch579_t;

extern const gpio_ops_t gpio_ch579_ops;

uint8_t gpio_ch579_init(gpio_ch579_t *self, const gpio_cfg_t *cfg);

#endif /* GPIO_CH579_H */
