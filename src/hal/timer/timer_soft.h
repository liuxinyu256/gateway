/**
 * timer_soft.h —— 软件定时器驱动 (多实例)
 * 多个软定时器共用一个硬件定时器: tick 源由程序员声明硬件 timer_t
 * (指定 id, 复用硬件驱动注册表) 后绑定, 软实例各自独立计数值/回调。
 */
#ifndef TIMER_SOFT_H
#define TIMER_SOFT_H
#include "timer.h"

typedef struct {
    uint32_t       counter;  /* 计数值: 共享 tick 每周期递增 */
    timer_callback cb;
    void          *ctx;
    uint8_t        running;
} soft_timer_t;

/* 绑定共享 tick 源 (硬件定时器, 程序员指定占哪个 id) */
void soft_timer_bind_tick(timer_t *tick);

/* 软定时器操作 (多实例: 实例由调用方声明, init 登记) */
void soft_timer_init(soft_timer_t *st);
void soft_timer_reset(soft_timer_t *st);
void soft_timer_stop(soft_timer_t *st);
void soft_timer_set_callback(soft_timer_t *st, timer_callback cb, void *ctx);

#endif
