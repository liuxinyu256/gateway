/**
 * main.c —— 入口
 */
#include "hvac_init.h"
#include "task.h"
#include "debug.h"

int main(void) {
    debug_init();
    debug_puts("[GW] boot\r\n");
    hvac_start();
    vTaskStartScheduler();
    for (;;);
}
