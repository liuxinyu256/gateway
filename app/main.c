/**
 * main.c —— CH579 网关入口
 */
#include "CH57x_common.h"
#include "frame_timer_hw.h"
#include "FreeRTOS.h"
#include "task.h"
#include "hvac_init.h"

int main(void) {
    hvac_start();
    vTaskStartScheduler();
    for (;;);
}

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
