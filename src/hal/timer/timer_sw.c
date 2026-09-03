/**
 * timer_sw.c —— 软件定时器驱动 (PC 模拟)
 *
 * 与 uart_ch579.c 同风格：提供 timer_ops_t + drv 私有数据。
 * 每个实例通过 timer_sw_bind() 绑定 id，主循环调用 timer_poll_all() 驱动。
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
#include <sys/time.h>
static uint64_t get_us(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000000ULL + (uint64_t)tv.tv_usec;
}
#endif

#define TIMER_MAX 8
#define TICK_US 1000

typedef struct {
    uint8_t  id;
    uint64_t next_tick;
} sw_timer_drv_t;

static sw_timer_drv_t drvs[TIMER_MAX];
static timer_t       *owners[TIMER_MAX];

static int sw_init(timer_t *t)
{
    sw_timer_drv_t *d = t ? (sw_timer_drv_t *)t->drv : NULL;
    if (!d || d->id >= TIMER_MAX)
        return -1;

    t->counter = 0;
    t->period  = TICK_US;
    t->running = 1;
    owners[d->id] = t;
    d->next_tick = get_us() + TICK_US;
    return 0;
}

static int sw_reset(timer_t *t)
{
    sw_timer_drv_t *d = t ? (sw_timer_drv_t *)t->drv : NULL;
    if (!d || d->id >= TIMER_MAX)
        return -1;

    t->counter = 0;
    t->running = 1;
    d->next_tick = get_us() + TICK_US;
    return 0;
}

static int sw_stop(timer_t *t)
{
    if (!t)
        return -1;

    t->counter = 0;
    t->running = 0;
    return 0;
}

static const timer_ops_t sw_timer_ops = {
    .init  = sw_init,
    .reset = sw_reset,
    .stop  = sw_stop,
};

void timer_sw_bind(timer_t *t, uint8_t id)
{
    if (!t || id >= TIMER_MAX)
        return;

    t->id  = id;
    t->ops = &sw_timer_ops;
    t->drv = &drvs[id];
    drvs[id].id = id;
    drvs[id].next_tick = 0;
    owners[id] = t;
}

/* 轮询驱动: 主循环定期调用, 等价于定时中断 */
void timer_poll_all(void)
{
    uint64_t now = get_us();
    for (uint8_t i = 0; i < TIMER_MAX; i++)
    {
        timer_t *t = owners[i];
        if (!t || !t->running)
            continue;

        sw_timer_drv_t *d = (sw_timer_drv_t *)t->drv;
        if (!d)
            continue;

        while (now >= d->next_tick)
        {
            d->next_tick += TICK_US;
            t->counter++;
            if (t->cb)
                t->cb(t->ctx);
            if (!t->running)
                break; /* 回调中可能 stop */
        }
    }
}
