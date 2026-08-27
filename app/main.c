/**
 * main.c —— 入口
 */
#include "hvac_init.h"
#include "debug.h"

int main(void) {
    debug_init();
    debug_puts("OK\r\n");
    hvac_start();
    vTaskStartScheduler();
    for (;;);
}
