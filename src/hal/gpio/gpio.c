/**
 * gpio.c —— GPIO 通用分发
 */
#include "gpio.h"

uint8_t gpio_init(gpio_t *g, const gpio_cfg_t *cfg)
{
    if (!g || !g->ops || !g->ops->init)
        return 1;
    return g->ops->init(g, cfg);
}

void gpio_set(gpio_t *g, uint8_t level)
{
    if (!g || !g->ops || !g->ops->set)
        return;
    g->ops->set(g, level);
}

void gpio_toggle(gpio_t *g)
{
    if (!g || !g->ops || !g->ops->toggle)
        return;
    g->ops->toggle(g);
}
