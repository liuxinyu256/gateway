/**
 * timer_sw.c —— 通用定时器驱动 (PC 模拟)
 * 注册表方式: 按定时器编号索引, 每项存对应定时器的绑定实例与下次 tick 时刻,
 * 主循环定期调用 timer_poll_all() 驱动 (微秒时钟, 每周期 counter 递增并调回调)。
 */
#include "timer.h"
#include <stddef.h>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
static uint64_t get_us(void)
{
    static LARGE_INTEGER f = {0};
    static int ok = 0;
    if (!ok)
    {
        QueryPerformanceFrequency(&f);
        ok = 1;
    }
    LARGE_INTEGER c;
    QueryPerformanceCounter(&c);
    return (uint64_t)((c.QuadPart * 1000000ULL) / f.QuadPart);
}
#else
#include <time.h>
static uint64_t get_us(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000ULL + (uint64_t)ts.tv_nsec / 1000ULL;
}
#endif

#define TIMER_MAX 8
#define TICK_US 1000

typedef struct
{
    uint32_t period;    /* 配置: 定时周期 (us) */
    timer_t *owner;     /* 绑定的实例 (timer_init 时登记) */
    uint64_t next_tick; /* 下次 tick 时刻 (us) */
} sw_reg_t;

static sw_reg_t regs[TIMER_MAX]; /* 注册表: timer0/1/... 各自的配置 */

void timer_init(timer_t *t)
{
    if (!t || t->id >= TIMER_MAX)
        return;
    sw_reg_t *r = &regs[t->id];
    t->counter = 0;
    t->period = TICK_US;
    t->running = 1;
    r->period = TICK_US;
    r->owner = t;
    r->next_tick = get_us() + TICK_US;
}

void timer_reset(timer_t *t)
{
    if (!t || t->id >= TIMER_MAX)
        return;
    sw_reg_t *r = &regs[t->id];
    t->counter = 0;
    t->running = 1;
    r->next_tick = get_us() + TICK_US;
}

void timer_stop(timer_t *t)
{
    if (!t || t->id >= TIMER_MAX)
        return;
    t->counter = 0;
    t->running = 0;
}

void timer_set_callback(timer_t *t, timer_callback cb, void *ctx)
{
    if (!t)
        return;
    t->cb = cb;
    t->ctx = ctx;
}

/* 轮询驱动: 主循环定期调用, 等价于定时中断 */
void timer_poll_all(void)
{
    uint64_t now = get_us();
    for (uint8_t i = 0; i < TIMER_MAX; i++)
    {
        sw_reg_t *r = &regs[i];
        timer_t *t = r->owner;
        if (!t || !t->running)
            continue;
        while (now >= r->next_tick)
        {
            r->next_tick += TICK_US;
            t->counter++;
            if (t->cb)
                t->cb(t->ctx);
            if (!t->running)
                break; /* 回调中可能 stop */
        }
    }
}
