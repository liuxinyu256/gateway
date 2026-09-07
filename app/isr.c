/**
 * isr.c —— 中断入口
 * 集中放定时器/UART 中断处理，保持 main.c 干净。
 * 未使用的外设中断全部提供空实现，避免进入默认死循环 B .
 */
#include "CH57x_common.h"
#include "timer.h"
#include "timer_instance.h"
#include "uart_ch579.h"
#include "gateway.h"
#include "sender.h"

/* FreeRTOS Run Time Stats 计数：TMR3 1ms 中断累加 */
volatile unsigned long g_rtos_run_time_ticks = 0UL;

static void rtos_run_time_tick(void *ctx)
{
    (void)ctx;
    g_rtos_run_time_ticks++;
}

void rtos_run_time_stats_init(void)
{
    timer_t *t = timer_get(3);
    if (!t)
        return;

    timer_set_callback(t, rtos_run_time_tick, NULL);
    timer_init(t);   /* 启动 TMR3 作为运行时间统计时基 */
}

/* ---- generic timer: TMR0-3 ---- */
void TMR0_IRQHandler(void) { timer_hw_isr(0); }
void TMR1_IRQHandler(void) { timer_hw_isr(1); }
void TMR2_IRQHandler(void) { timer_hw_isr(2); }
void TMR3_IRQHandler(void) { timer_hw_isr(3); }

/* ---- UART ---- */
void UART0_IRQHandler(void) {
    ch579_uart_irq_handler(0);

    module_t *m = gateway_module(0);
    if (m && m->sender)
        sender_isr(m->sender);
}

void UART1_IRQHandler(void) {
    ch579_uart_irq_handler(1);

    module_t *m = gateway_module(1);
    if (m && m->sender)
        sender_isr(m->sender);
}

void UART2_IRQHandler(void) { ch579_uart_irq_handler(2); }
void UART3_IRQHandler(void) { ch579_uart_irq_handler(3); }

/* ---- 未使用外设中断：空实现，防止默认 B . 死循环 ---- */
void GPIO_IRQHandler(void)  { }
void SLAVE_IRQHandler(void) { }
void SPI0_IRQHandler(void)  { }
void BB_IRQHandler(void)    { }
#ifndef BLE_ENABLE
void LLE_IRQHandler(void)   { }
#endif
void USB_IRQHandler(void)   { }
void ETH_IRQHandler(void)   { }
#ifndef BLE_ENABLE
void RTC_IRQHandler(void)   { }
#endif
void ADC_IRQHandler(void)   { }
void SPI1_IRQHandler(void)  { }
void LED_IRQHandler(void)   { }
void WDT_IRQHandler(void)   { }
