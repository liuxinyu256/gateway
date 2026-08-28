/**
 * main.c —— 入口
 */
#include "hvac_init.h"

int main(void) {
    hvac_start();
    vTaskStartScheduler();
    for (;;);
}
