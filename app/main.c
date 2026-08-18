/**
 * main.c —— CH579 网关入口
 */
#include "CH57x_common.h"
#include "frame_timer_hw.h"
#include "uart_hw.h"
#include "timer_hw.h"
#include "FreeRTOS.h"
#include "task.h"
#include "hvac_init.h"

int main(void) {
    hvac_start();
    vTaskStartScheduler();
    for (;;);
}

/* ---- frame timer: TMR0-1 ---- */
void TMR0_IRQHandler(void) {
    if (TMR0_GetITFlag(TMR0_3_IT_CYC_END)) {
        TMR0_ClearITFlag(TMR0_3_IT_CYC_END);
        frame_timer_hw_isr(0);
    }
}

void TMR1_IRQHandler(void) {
    if (TMR1_GetITFlag(TMR0_3_IT_CYC_END)) {
        TMR1_ClearITFlag(TMR0_3_IT_CYC_END);
        frame_timer_hw_isr(1);
    }
}

/* ---- generic timer: TMR2-3 ---- */
void TMR2_IRQHandler(void) { timer_hw_isr(2); }
void TMR3_IRQHandler(void) { timer_hw_isr(3); }

/* ---- UART ---- */
void UART0_IRQHandler(void) { uart_hw_isr(0); }
void UART1_IRQHandler(void) { uart_hw_isr(1); }
void UART2_IRQHandler(void) { uart_hw_isr(2); }
void UART3_IRQHandler(void) { uart_hw_isr(3); }
