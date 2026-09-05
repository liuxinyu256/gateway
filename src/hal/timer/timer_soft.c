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
static timer_t       *s_tick;             /* 共享硬件 tick 源，只绑定一次 */

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
    /* 共享硬件定时器一旦开启就作为时基持续运行，不能被 soft_timer_stop 关闭 */
    if (!t || s_tick)
        return;                 /* 多个物理层可重复调用，只绑第一个 */
    s_tick = t;
    timer_set_callback(t, soft_tick, NULL);
    timer_init(t);              /* 开启硬件定时中断，之后不再 stop */
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

/* 只清/启停本软实例，绝不操作共享硬件 tick */
void soft_timer_reset(soft_timer_t *st)
{
    if (!st) return;
    st->counter = 0;
    st->running = 1;
}

void soft_timer_stop(soft_timer_t *st)
{
    if (!st) return;
    st->counter = 0;   /* 清的是软实例计数值 */
    st->running = 0;   /* 只停软实例，不影响硬件定时器 */
}

void soft_timer_set_callback(soft_timer_t *st, timer_callback cb, void *ctx)
{
    if (!st) return;
    st->cb  = cb;
    st->ctx = ctx;
}
