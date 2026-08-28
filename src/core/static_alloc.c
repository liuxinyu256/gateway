/**
 * static_alloc.c —— FreeRTOS 静态分配支持
 *
 * 提供空闲任务和定时器守护任务的静态 TCB/栈。
 */
#include "FreeRTOS.h"
#include "task.h"

/* 空闲任务静态资源 */
static StackType_t  idle_task_stack[configMINIMAL_STACK_SIZE];
static StaticTask_t idle_task_tcb;

/* 定时器守护任务静态资源 */
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
