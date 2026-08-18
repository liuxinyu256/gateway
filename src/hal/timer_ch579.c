/**
 * timer_ch579.c —— 定时器驱动实现 (CH579)
 * 16 个硬件操作函数名与其它芯片实现一致 (接口名固定, 不同芯片文件不同时编译),
 * 体按芯片写; 注册表 timer_hw_regs 由本文件提供。
 * 整个文件由 __CH579__ 宏包住: 未定义时编译为空, 误加进源列表也不生效。
 */
#include "timer.h"

#ifdef __CH579__
#include "CH57x_common.h"

static void t0_init(void)
{
    TMR0_TimerInit(FREQ_SYS / 1000);
    TMR0_ITCfg(ENABLE, TMR0_3_IT_CYC_END);
    NVIC_EnableIRQ(TMR0_IRQn);
}
static void t0_restart(void)
{
    TMR0_Disable();
    TMR0_Enable();
}
static void t0_clear_count(void)
{
    R8_TMR0_CTRL_MOD = RB_TMR_ALL_CLEAR; /* 清空硬件计数寄存器 */
    R8_TMR0_CTRL_MOD = RB_TMR_COUNT_EN;
}
static void t0_stop(void) { TMR0_Disable(); }
static void t0_clear(void) { TMR0_ClearITFlag(TMR0_3_IT_CYC_END); }

static void t1_init(void)
{
    TMR1_TimerInit(FREQ_SYS / 1000);
    TMR1_ITCfg(ENABLE, TMR0_3_IT_CYC_END);
    NVIC_EnableIRQ(TMR1_IRQn);
}
static void t1_restart(void)
{
    TMR1_Disable();
    TMR1_Enable();
}
static void t1_clear_count(void)
{
    R8_TMR1_CTRL_MOD = RB_TMR_ALL_CLEAR; /* 清空硬件计数寄存器 */
    R8_TMR1_CTRL_MOD = RB_TMR_COUNT_EN;
}
static void t1_stop(void) { TMR1_Disable(); }
static void t1_clear(void) { TMR1_ClearITFlag(TMR0_3_IT_CYC_END); }

static void t2_init(void)
{
    TMR2_TimerInit(FREQ_SYS / 1000);
    TMR2_ITCfg(ENABLE, TMR0_3_IT_CYC_END);
    NVIC_EnableIRQ(TMR2_IRQn);
}
static void t2_restart(void)
{
    TMR2_Disable();
    TMR2_Enable();
}
static void t2_clear_count(void)
{
    R8_TMR2_CTRL_MOD = RB_TMR_ALL_CLEAR; /* 清空硬件计数寄存器 */
    R8_TMR2_CTRL_MOD = RB_TMR_COUNT_EN;
}
static void t2_stop(void) { TMR2_Disable(); }
static void t2_clear(void) { TMR2_ClearITFlag(TMR0_3_IT_CYC_END); }

static void t3_init(void)
{
    TMR3_TimerInit(FREQ_SYS / 1000);
    TMR3_ITCfg(ENABLE, TMR0_3_IT_CYC_END);
    NVIC_EnableIRQ(TMR3_IRQn);
}
static void t3_restart(void)
{
    TMR3_Disable();
    TMR3_Enable();
}
static void t3_clear_count(void)
{
    R8_TMR3_CTRL_MOD = RB_TMR_ALL_CLEAR; /* 清空硬件计数寄存器 */
    R8_TMR3_CTRL_MOD = RB_TMR_COUNT_EN;
}
static void t3_stop(void) { TMR3_Disable(); }
static void t3_clear(void) { TMR3_ClearITFlag(TMR0_3_IT_CYC_END); }

/* CH579 硬件注册表 */
timer_reg_t timer_hw_regs[TIMER_HW_MAX] = {
    { .period = 1000, .init = t0_init, .restart = t0_restart, .stop = t0_stop, .clear_count = t0_clear_count, .clear_it = t0_clear },
    { .period = 1000, .init = t1_init, .restart = t1_restart, .stop = t1_stop, .clear_count = t1_clear_count, .clear_it = t1_clear },
    { .period = 1000, .init = t2_init, .restart = t2_restart, .stop = t2_stop, .clear_count = t2_clear_count, .clear_it = t2_clear },
    { .period = 1000, .init = t3_init, .restart = t3_restart, .stop = t3_stop, .clear_count = t3_clear_count, .clear_it = t3_clear },
};

#endif /* __CH579__ */
