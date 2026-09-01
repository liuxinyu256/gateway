/**
 * heap.c —— FreeRTOS 堆内存定义
 *
 * 使用 configAPPLICATION_ALLOCATED_HEAP=1 由应用定义堆数组，
 * 并放到 .heap 段，确保链接到 RAM1 低区，避免和 BLE 预留区冲突。
 */
#include "FreeRTOS.h"

#if ( configAPPLICATION_ALLOCATED_HEAP == 1 )
__attribute__((section(".heap")))
uint8_t ucHeap[ configTOTAL_HEAP_SIZE ];
#endif

#if ( configSUPPORT_STATIC_ALLOCATION == 1 )
/* Idle 任务静态内存 */
static StackType_t idle_task_stack[ configMINIMAL_STACK_SIZE ];
static StaticTask_t idle_task_tcb;

/* Timer 服务任务静态内存 */
static StackType_t timer_task_stack[ configTIMER_TASK_STACK_DEPTH ];
static StaticTask_t timer_task_tcb;

void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer,
                                    StackType_t **ppxIdleTaskStackBuffer,
                                    uint32_t *pulIdleTaskStackSize )
{
    *ppxIdleTaskTCBBuffer = &idle_task_tcb;
    *ppxIdleTaskStackBuffer = idle_task_stack;
    *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}

void vApplicationGetTimerTaskMemory( StaticTask_t **ppxTimerTaskTCBBuffer,
                                     StackType_t **ppxTimerTaskStackBuffer,
                                     uint32_t *pulTimerTaskStackSize )
{
    *ppxTimerTaskTCBBuffer = &timer_task_tcb;
    *ppxTimerTaskStackBuffer = timer_task_stack;
    *pulTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH;
}
#endif /* configSUPPORT_STATIC_ALLOCATION */
