/**
 * timer_hw.c —— 硬件定时器通用封装 (平台无关)
 *
 * 与 uart.c 的角色一致：只做通用分发。
 * 平台绑定/解绑/查实例/清中断由 timer_ch579.c 等实现。
 */
#include "timer.h"

int timer_hw_create(timer_t *t, uint8_t hw_id)
{
    if (!t || timer_hw_bind(t, hw_id) != 0)
        return -1;

    return timer_init(t);
}

void timer_hw_destroy(timer_t *t)
{
    if (!t)
        return;

    timer_stop(t);
    timer_hw_unbind(t);
}

/* 定时中断统一入口 (平台 TMR 中断处理调用) */
void timer_hw_isr(uint8_t id)
{
    timer_t *t = timer_hw_get(id);
    if (!t)
        return;

    timer_hw_clear_it(id);
    t->counter++;
    if (t->cb)
        t->cb(t->ctx);
}
