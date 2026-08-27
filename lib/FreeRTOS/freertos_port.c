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
