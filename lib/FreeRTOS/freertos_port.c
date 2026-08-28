/********************************** (C) COPYRIGHT *******************************
 * File Name          : freertos_port.c
 * Description        : FreeRTOS V11.1.0 统一编译入口
 *
 * 聚合所有 FreeRTOS 源文件为一个编译单元，
 * 只需在 Keil 中添加这一个文件。
 *******************************************************************************/

#include "Source/tasks.c"
#include "Source/queue.c"
#include "Source/list.c"
#include "Source/timers.c"
#include "Source/event_groups.c"
#include "Source/stream_buffer.c"
#include "Portable/RVDS/ARM_CM0/port.c"
#include "Portable/MemMang/heap_4.c"

/* 静态分配：空闲任务与定时器守护任务内存 */
static StackType_t  idle_task_stack[configMINIMAL_STACK_SIZE];
static StaticTask_t idle_task_tcb;

static StackType_t  timer_task_stack[configTIMER_TASK_STACK_DEPTH];
static StaticTask_t timer_task_tcb;

void vApplicationGetIdleTaskMemory(StaticTask_t **ppxTCBBuffer,
                                   StackType_t **ppxStackBuffer,
                                   uint32_t *pulStackDepth)
{
    *ppxTCBBuffer   = &idle_task_tcb;
    *ppxStackBuffer = idle_task_stack;
    *pulStackDepth  = configMINIMAL_STACK_SIZE;
}

void vApplicationGetTimerTaskMemory(StaticTask_t **ppxTCBBuffer,
                                    StackType_t **ppxStackBuffer,
                                    uint32_t *pulStackDepth)
{
    *ppxTCBBuffer   = &timer_task_tcb;
    *ppxStackBuffer = timer_task_stack;
    *pulStackDepth  = configTIMER_TASK_STACK_DEPTH;
}
