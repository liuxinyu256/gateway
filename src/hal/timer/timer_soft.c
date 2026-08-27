/**
 * timer_soft.c —— 软件定时器驱动 (多实例)
 * 多个软实例共用一个硬件定时器: tick 源经 timer.h 接口注册进硬件驱动
 * 注册表 (复用硬件定时器), 其中断回调分发到所有软实例。
 * 回调在定时中断上下文, 须轻量。
 */
#include "timer_soft.h"
#include <stddef.h>

#define SOFT_MAX 8

static soft_timer_t *soft_regs[SOFT_MAX]; /* 软实例注册表 */

/* 硬件定时中断 → 分发到所有软实例 */
static void soft_tick(void *ctx)
{
    (void)ctx;
    for (uint8_t i = 0; i < SOFT_MAX; i++)
    {
        soft_timer_t *st = soft_regs[i];
        if (!st || !st->running) continue;
        st->counter++;
        if (st->cb) st->cb(st->ctx);
    }
}

void soft_timer_bind_tick(timer_t *t)
{
    if (!t) return;
    timer_set_callback(t, soft_tick, NULL);
    timer_init(t);              /* 复用硬件注册表: 开启定时中断 */
}

void soft_timer_init(soft_timer_t *st)
{
    if (!st) return;
    for (uint8_t i = 0; i < SOFT_MAX; i++)
    {
        if (!soft_regs[i])
        {
            st->counter = 0;
            st->running = 1;
            soft_regs[i] = st;
            return;
        }
    }
}

void soft_timer_reset(soft_timer_t *st)
{
    if (!st) return;
    st->counter = 0;
    st->running = 1;
}

void soft_timer_stop(soft_timer_t *st)
{
    if (!st) return;
    st->counter = 0;
    st->running = 0;
}

void soft_timer_set_callback(soft_timer_t *st, timer_callback cb, void *ctx)
{
    if (!st) return;
    st->cb  = cb;
    st->ctx = ctx;
}
