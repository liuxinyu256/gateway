/**
 * timer.h —— 通用定时器驱动接口 (平台无关)
 *
 * 与 uart.h 保持同一种对象 + ops 风格：
 *   - timer_t 自带 ops 虚表和 drv 私有数据
 *   - 平台实现提供 timer_ops_t，通过 drv 保存实例/硬件编号
 *   - 使用方只依赖 timer_t / timer_init / timer_reset / timer_stop
 *
 * 使用方: receiver_timeout (帧间隙超时)、模块轮询/重试等。
 */
#ifndef TIMER_H
#define TIMER_H
#include <stdint.h>

typedef struct timer timer_t;
typedef void (*timer_callback)(void *ctx);

typedef struct timer_ops {
    int (*init)(timer_t *t);   /* 初始化定时器并开启定时中断 */
    int (*reset)(timer_t *t);  /* 重置计数值 */
    int (*stop)(timer_t *t);   /* 关闭定时器并清空计数值 */
} timer_ops_t;

struct timer
{
    const timer_ops_t *ops; /* 平台驱动操作表 */
    void              *drv; /* 平台私有数据 (例如硬件编号/软件实例) */

    uint8_t  id;       /* 绑定的定时器编号 (创建时绑定, 驱动解释) */
    uint32_t counter;  /* 计数值: 定时中断每周期递增 */
    timer_callback cb;
    void          *ctx;
    uint32_t period;   /* 周期 (驱动配置) */
    uint8_t  running;
};

/* 通用定时器驱动接口 (实现: timer.c 统一分发到 t->ops) */
int  timer_init(timer_t *t);  /* 初始化定时器并开启定时中断, 0=成功 */
int  timer_reset(timer_t *t); /* 重置计数值, 0=成功 */
int  timer_stop(timer_t *t);  /* 关闭定时器并清空计数值, 0=成功 */
void timer_set_callback(timer_t *t, timer_callback cb, void *ctx);

#ifdef FAKE_FREERTOS
/* PC 模拟: 绑定软件定时器实例 (id 对应 timer0~timer3) */
void timer_sw_bind(timer_t *t, uint8_t id);
void timer_poll_all(void);
#else
/* 硬件定时器创建/释放 */
int  timer_hw_create(timer_t *t, uint8_t hw_id);
void timer_hw_destroy(timer_t *t);

/* 平台实现 (timer_ch579.c 等) 提供 */
int      timer_hw_bind(timer_t *t, uint8_t hw_id);
void     timer_hw_unbind(timer_t *t);
timer_t *timer_hw_get(uint8_t id);
void     timer_hw_clear_it(uint8_t id);

/* 定时中断统一入口 (平台 TMR 中断处理调用) */
void timer_hw_isr(uint8_t id);
#endif

#endif
