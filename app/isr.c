/**
 * isr.c —— 中断入口
 * 集中放定时器/UART 中断处理，保持 main.c 干净。
 */
#include "CH57x_common.h"
#include "timer.h"
#include "uart_ch579.h"
#include "gateway.h"
#include "sender.h"

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

void UART1_IRQHandler(void) { ch579_uart_irq_handler(1); }

void UART2_IRQHandler(void) { ch579_uart_irq_handler(2); }
void UART3_IRQHandler(void) { ch579_uart_irq_handler(3); }
