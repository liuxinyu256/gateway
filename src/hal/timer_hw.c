/**
 * timer_hw.c —— 通用定时器驱动 (查表分派, 平台无关)
 * 硬件操作与注册表由芯片实现文件提供 (timer_ch579.c 等, 编译时选一个),
 * 本文件只按 id 查表分派。
 */
#include "timer.h"

static uint8_t s_hw_used; /* 低 4 位: bit0=TMR0, bit1=TMR1, bit2=TMR2, bit3=TMR3 */

void timer_init(timer_t *t)
{
    if (!t || t->id >= TIMER_HW_MAX)
        return;
    timer_reg_t *r = &timer_hw_regs[t->id];
    if (r->owner && r->owner != t)
        return; /* 已被别的实例占用 */
    t->counter = 0;
    t->period = r->period;
    t->running = 1;
    r->owner = t; /* 登记绑定 */
    r->init();    /* 开启定时中断 */
}

void timer_reset(timer_t *t)
{
    if (!t || t->id >= TIMER_HW_MAX)
        return;
    timer_reg_t *r = &timer_hw_regs[t->id];
    t->counter = 0;
    t->running = 1;
    if (r->clear_count)
        r->clear_count(); /* 清空硬件计数寄存器并重新计时 */
    else if (r->restart)
        r->restart();
}

void timer_stop(timer_t *t)
{
    if (!t || t->id >= TIMER_HW_MAX)
        return;
    timer_reg_t *r = &timer_hw_regs[t->id];
    r->stop();      /* 关闭定时器 */
    r->owner = NULL; /* 解除绑定 */
    t->counter = 0; /* 清空计数值 */
    t->running = 0;
}

void timer_set_callback(timer_t *t, timer_callback cb, void *ctx)
{
    if (!t)
        return;
    t->cb = cb;
    t->ctx = ctx;
}

int timer_hw_create(timer_t *t, uint8_t hw_id)
{
    if (!t || hw_id >= TIMER_HW_MAX)
        return -1;

    uint8_t bit = (uint8_t)(1u << hw_id);
    if (s_hw_used & bit)
        return -1; /* 已被占用 */
    if (timer_hw_regs[hw_id].owner)
        return -1; /* 双保险 */

    s_hw_used |= bit;
    t->id = hw_id;
    timer_init(t);
    return 0;
}

void timer_hw_destroy(timer_t *t)
{
    if (!t || t->id >= TIMER_HW_MAX)
        return;

    uint8_t bit = (uint8_t)(1u << t->id);
    if (!(s_hw_used & bit))
        return;

    timer_stop(t);
    timer_hw_regs[t->id].owner = NULL;
    s_hw_used &= (uint8_t)~bit;
}

/* 定时中断统一入口 (平台 TMR 中断处理调用): 计数值递增, 到点调回调 */
void timer_isr(uint8_t id)
{
    if (id >= TIMER_HW_MAX)
        return;
    timer_reg_t *r = &timer_hw_regs[id];
    r->clear_it();
    timer_t *t = r->owner;
    if (!t)
        return;
    t->counter++;
    if (t->cb)
        t->cb(t->ctx);
}
