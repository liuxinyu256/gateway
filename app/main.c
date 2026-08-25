/**
 * main.c —— CH579 网关入口
 */
#include "CH57x_common.h"
#include "timer.h"
#include "uart_ch579.h"
#include "gateway.h"
#include "FreeRTOS.h"
#include "task.h"
#include "hvac_init.h"

int main(void) {
    hvac_start();
    vTaskStartScheduler();
    for (;;);
}

/* ---- generic timer: TMR0-3 ---- */
void TMR0_IRQHandler(void) { timer_hw_isr(0); }
void TMR1_IRQHandler(void) { timer_hw_isr(1); }
void TMR2_IRQHandler(void) { timer_hw_isr(2); }
void TMR3_IRQHandler(void) { timer_hw_isr(3); }

/* ---- UART ---- */
void UART0_IRQHandler(void) { ch579_uart_irq_handler(0); }

void UART1_IRQHandler(void) {
    ch579_uart_irq_handler(1);

    module_t *m = gateway_module(0);
    if (m && m->sender)
        sender_uart_isr(m->sender);
}

void UART2_IRQHandler(void) { ch579_uart_irq_handler(2); }
void UART3_IRQHandler(void) { ch579_uart_irq_handler(3); }
