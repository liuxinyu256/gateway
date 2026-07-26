/********************************** (C) COPYRIGHT *******************************
 * File Name          : freertos_port.c
 * Description        : FreeRTOS V11.1.0 统一编译入口
 *
 * 聚合所有 FreeRTOS 源文件为一个编译单元，
 * 只需在 Keil 中添加这一个文件。
 *******************************************************************************/

#include "FreeRTOS/Source/tasks.c"
#include "FreeRTOS/Source/queue.c"
#include "FreeRTOS/Source/list.c"
#include "FreeRTOS/Source/timers.c"
#include "FreeRTOS/Source/event_groups.c"
#include "FreeRTOS/Source/stream_buffer.c"
#include "FreeRTOS/Portable/RVDS/ARM_CM0/port.c"
#include "FreeRTOS/Portable/MemMang/heap_4.c"
