/**
 * timer.c —— 通用定时器驱动分发 (平台无关)
 *
 * 只做 ops 分发，和 uart.c 的角色一致。
 */
#include "timer.h"

int timer_init(timer_t *t)
{
    if (!t || !t->ops || !t->ops->init)
        return -1;
    return t->ops->init(t);
}

int timer_reset(timer_t *t)
{
    if (!t || !t->ops || !t->ops->reset)
        return -1;
    return t->ops->reset(t);
}

int timer_stop(timer_t *t)
{
    if (!t || !t->ops || !t->ops->stop)
        return -1;
    return t->ops->stop(t);
}

void timer_set_callback(timer_t *t, timer_callback cb, void *ctx)
{
    if (!t) return;
    t->cb  = cb;
    t->ctx = ctx;
}
