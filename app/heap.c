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
