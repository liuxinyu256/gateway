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

void gpio_deinit(gpio_t *g)
{
    if (!g || !g->ops || !g->ops->deinit)
        return;
    g->ops->deinit(g);
}

void gpio_reset(gpio_t *g)
{
    if (!g || !g->ops || !g->ops->reset)
        return;
    g->ops->reset(g);
}

void gpio_set(gpio_t *g, gpio_level_t level)
{
    if (!g || !g->ops || !g->ops->set)
        return;
    g->ops->set(g, level);
}

gpio_level_t gpio_get(gpio_t *g)
{
    if (!g || !g->ops || !g->ops->get)
        return GPIO_LEVEL_LOW;
    return g->ops->get(g);
}

void gpio_toggle(gpio_t *g)
{
    if (!g || !g->ops || !g->ops->toggle)
        return;
    g->ops->toggle(g);
}
