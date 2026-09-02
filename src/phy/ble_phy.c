/**
 * ble_phy.c —— BLE 物理层装配
 *
 * 接入 CH57xBLE 协议栈：
 *   - 初始化 BLE
 *   - 创建 FreeRTOS 任务循环 TMOS_SystemProcess()
 *   - MEM_BUF 固定放在 0x20003800（BLE 保留区）
 *
 * 未定义 BLE_ENABLE 时编译为空实现，避免影响非 BLE 构建。
 */
#include "ble_phy.h"

#ifdef BLE_ENABLE
#include "CONFIG.h"
#include "HAL.h"
#include "peripheral.h"
#include "FreeRTOS.h"
#include "task.h"

/* BLE 协议栈内存，固定放在 0x20003800，不能动 */
__align(4) u32 MEM_BUF[BLE_MEMHEAP_SIZE / 4] __attribute__((at(0x20003800)));

/* TMOS 调度任务：BLE 协议栈自己跑在自己的任务里 */
static void ble_tmos_task(void *arg)
{
    (void)arg;

    for (;;) {
        TMOS_SystemProcess();
        vTaskDelay(pdMS_TO_TICKS(1));   /* 让出 CPU，避免饿死业务任务 */
    }
}

uint8_t ble_phy_init(void)
{
    CH57X_BLEInit();
    HAL_Init();
    GAPRole_PeripheralInit();
    Peripheral_Init();

    xTaskCreate(ble_tmos_task, "ble", 256, NULL, 2, NULL);
    return 0;
}
#else
uint8_t ble_phy_init(void)
{
    /* BLE_ENABLE 未开启：空实现 */
    return 0;
}
#endif
