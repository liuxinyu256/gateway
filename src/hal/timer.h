/**
 * timer.h —— 通用定时器驱动接口 (平台无关)
 *
 * 通用定时器驱动: 占用哪个定时器由程序员决定 (id 指定),
 * 实例由程序员声明并注入使用方; 接口函数即驱动入口,
 * 实现文件由编译时选定, 直接对接硬件 (定时中断每周期递增 counter)。
 * 使用方: receiver_timeout (帧间隙超时)、模块轮询/重试等。
 */
#ifndef TIMER_H
#define TIMER_H
#include <stdint.h>

typedef struct timer timer_t;
typedef void (*timer_callback)(void *ctx);

struct timer
{
    uint8_t id;       /* 绑定的定时器编号 (程序员决定, 驱动解释) */
    uint32_t counter; /* 计数值: 定时中断每周期递增 */
    timer_callback cb;
    void *ctx;
    uint32_t period; /* 周期 (驱动配置) */
    uint8_t running;
};

/* 定时器驱动接口 (实现: 编译时选定的驱动文件, 直接对接硬件) */
void timer_init(timer_t *t);  /* 初始化定时器并开启定时中断 */
void timer_reset(timer_t *t); /* 重置计数值 */
void timer_stop(timer_t *t);  /* 关闭定时器并清空计数值 */
void timer_set_callback(timer_t *t, timer_callback cb, void *ctx);

#ifndef FAKE_FREERTOS
/* 硬件定时器创建/释放: 用 bitmap 低 4 位记录 TMR0~TMR3 占用 */
int  timer_hw_create(timer_t *t, uint8_t hw_id);
void timer_hw_destroy(timer_t *t);
#endif

/* ---- 硬件注册表 (由各芯片实现文件提供, 编译时选一个) ---- */
#define TIMER_HW_MAX 4

typedef struct
{
    uint32_t period;        /* 配置: 定时周期 (us) */
    void (*init)(void);     /* 配置寄存器并开启定时中断 */
    void (*restart)(void);  /* 重新启动计时 */
    void (*stop)(void);     /* 关闭定时器 */
    void (*clear_count)(void); /* 清空硬件计数寄存器 (不是软件 counter) */
    void (*clear_it)(void); /* 清定时中断标志 */
    timer_t *owner;         /* 绑定的实例 (timer_init 时登记) */
} timer_reg_t;

extern timer_reg_t timer_hw_regs[TIMER_HW_MAX];

#endif
