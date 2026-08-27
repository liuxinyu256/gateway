/**
 * timer_ch579.c —— CH579 硬件定时器驱动实现
 *
 * 与 uart_ch579.c 同风格：提供 timer_ops_t + drv 私有数据，
 * 全局实例由 timer_instance.c 绑定，timer_hw_get/clear_it 供通用层使用。
 * 整个文件由 __CH579__ 宏包住: 未定义时编译为空。
 */
#include "timer.h"
#include "timer_ch579.h"

#ifdef __CH579__
#include "CH57x_common.h"

#define TIMER_HW_MAX 4

ch579_timer_drv_t ch579_timer_drvs[TIMER_HW_MAX] = {
    { .hw_id = 0 },
    { .hw_id = 1 },
    { .hw_id = 2 },
    { .hw_id = 3 },
};

static timer_t *owners[TIMER_HW_MAX];

static void ch579_hw_init(uint8_t id)
{
    switch (id) {
    case 0:
        TMR0_TimerInit(FREQ_SYS / 1000);
        TMR0_ITCfg(ENABLE, TMR0_3_IT_CYC_END);
        NVIC_EnableIRQ(TMR0_IRQn);
        break;
    case 1:
        TMR1_TimerInit(FREQ_SYS / 1000);
        TMR1_ITCfg(ENABLE, TMR0_3_IT_CYC_END);
        NVIC_EnableIRQ(TMR1_IRQn);
        break;
    case 2:
        TMR2_TimerInit(FREQ_SYS / 1000);
        TMR2_ITCfg(ENABLE, TMR0_3_IT_CYC_END);
        NVIC_EnableIRQ(TMR2_IRQn);
        break;
    case 3:
        TMR3_TimerInit(FREQ_SYS / 1000);
        TMR3_ITCfg(ENABLE, TMR0_3_IT_CYC_END);
        NVIC_EnableIRQ(TMR3_IRQn);
        break;
    default:
        break;
    }
}

static void ch579_hw_clear_count(uint8_t id)
{
    switch (id) {
    case 0:
        R8_TMR0_CTRL_MOD = RB_TMR_ALL_CLEAR;
        R8_TMR0_CTRL_MOD = RB_TMR_COUNT_EN;
        break;
    case 1:
        R8_TMR1_CTRL_MOD = RB_TMR_ALL_CLEAR;
        R8_TMR1_CTRL_MOD = RB_TMR_COUNT_EN;
        break;
    case 2:
        R8_TMR2_CTRL_MOD = RB_TMR_ALL_CLEAR;
        R8_TMR2_CTRL_MOD = RB_TMR_COUNT_EN;
        break;
    case 3:
        R8_TMR3_CTRL_MOD = RB_TMR_ALL_CLEAR;
        R8_TMR3_CTRL_MOD = RB_TMR_COUNT_EN;
        break;
    default:
        break;
    }
}

static void ch579_hw_stop(uint8_t id)
{
    switch (id) {
    case 0: TMR0_Disable(); break;
    case 1: TMR1_Disable(); break;
    case 2: TMR2_Disable(); break;
    case 3: TMR3_Disable(); break;
    default: break;
    }
}

static void ch579_hw_clear_it(uint8_t id)
{
    switch (id) {
    case 0: TMR0_ClearITFlag(TMR0_3_IT_CYC_END); break;
    case 1: TMR1_ClearITFlag(TMR0_3_IT_CYC_END); break;
    case 2: TMR2_ClearITFlag(TMR0_3_IT_CYC_END); break;
    case 3: TMR3_ClearITFlag(TMR0_3_IT_CYC_END); break;
    default: break;
    }
}

static int ch579_init(timer_t *t)
{
    ch579_timer_drv_t *d = t ? (ch579_timer_drv_t *)t->drv : NULL;
    if (!d || d->hw_id >= TIMER_HW_MAX)
        return -1;

    if (owners[d->hw_id] && owners[d->hw_id] != t)
        return -1; /* 已被别的实例占用 */

    owners[d->hw_id] = t;
    t->counter = 0;
    t->period  = 1000;
    t->running = 1;
    ch579_hw_init(d->hw_id);
    return 0;
}

static int ch579_reset(timer_t *t)
{
    ch579_timer_drv_t *d = t ? (ch579_timer_drv_t *)t->drv : NULL;
    if (!d || d->hw_id >= TIMER_HW_MAX)
        return -1;

    t->counter = 0;
    t->running = 1;
    ch579_hw_clear_count(d->hw_id);
    return 0;
}

static int ch579_stop(timer_t *t)
{
    ch579_timer_drv_t *d = t ? (ch579_timer_drv_t *)t->drv : NULL;
    if (!d || d->hw_id >= TIMER_HW_MAX)
        return -1;

    ch579_hw_stop(d->hw_id);
    t->counter = 0;
    t->running = 0;
    return 0;
}

const timer_ops_t ch579_timer_ops = {
    .init  = ch579_init,
    .reset = ch579_reset,
    .stop  = ch579_stop,
};

timer_t *timer_hw_get(uint8_t id)
{
    if (id >= TIMER_HW_MAX)
        return NULL;
    return owners[id];
}

void timer_hw_clear_it(uint8_t id)
{
    if (id < TIMER_HW_MAX)
        ch579_hw_clear_it(id);
}

#endif /* __CH579__ */
