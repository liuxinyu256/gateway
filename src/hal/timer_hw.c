/**
 * timer_hw.c —— 硬件定时器通用封装 (平台无关)
 *
 * 基于全局实例 timer0..timer3，与 uart_get() 同样式。
 */
#include "timer.h"
#include "timer_instance.h"
#include <stddef.h>

timer_t *timer_hw_create(uint8_t hw_id)
{
    timer_t *t = timer_get(hw_id);
    if (!t)
        return NULL;

    if (timer_init(t) != 0)
        return NULL;

    return t;
}

void timer_hw_destroy(uint8_t hw_id)
{
    timer_t *t = timer_get(hw_id);
    if (t)
        timer_stop(t);
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
