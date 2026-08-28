/**
 * main.c —— 入口
 */
#include "hvac_init.h"
#include "debug_module.h"

int main(void) {
    hvac_start();
    debug_module_start();
    vTaskStartScheduler();
    for (;;);
}
