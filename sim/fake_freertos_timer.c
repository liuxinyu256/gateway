/**
 * fake_freertos_timer.c —— 统一定时器接口 (FreeRTOS software timer) 的 PC 实现
 *
 * 注册表与时钟跨编译单元共享: receiver_timeout 与单元测试各自编译,
 * 通过同一份 fake_timers / fake_now 驱动帧间隙超时逻辑。
 */
#include "fake_freertos.h"

fake_timer_t fake_timers[FAKE_TIMER_MAX];
int          fake_timer_count;
TickType_t   fake_now;
