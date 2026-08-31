/**
 * gpio_instance.c —— GPIO 全局实例实现
 */
#include "gpio_instance.h"
#include <stddef.h>

#ifdef __CH579__
#include "gpio_ch579.h"

static gpio_ch579_t gpio_pa[GPIO_PIN_COUNT_A];
static gpio_ch579_t gpio_pb[GPIO_PIN_COUNT_B];

gpio_t *gpio_get_instance(uint8_t port, uint8_t pin)
{
    gpio_ch579_t *g;

    if (port == GPIO_PORT_B) {
        if (pin >= GPIO_PIN_COUNT_B)
            return NULL;
        g = &gpio_pb[pin];
    } else {
        if (pin >= GPIO_PIN_COUNT_A)
            return NULL;
        g = &gpio_pa[pin];
    }

    /* 懒绑定：第一次获取时挂上 CH579 驱动 ops */
    if (!g->base.ops)
        g->base.ops = &gpio_ch579_ops;
    return &g->base;
}

#else

gpio_t *gpio_get_instance(uint8_t port, uint8_t pin)
{
    (void)port;
    (void)pin;
    return NULL;
}

#endif /* __CH579__ */
