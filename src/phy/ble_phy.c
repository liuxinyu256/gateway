/**
 * ble_phy.c —— BLE 物理层装配（事件驱动版）
 *
 * 思路：
 *   - BLE 初始化放在 FreeRTOS 任务里，避免调度器启动前卡死
 *   - BLE 任务平时阻塞在任务通知上，不占 CPU
 *   - RTC/LLE 中断通过 ble_phy_notify_from_isr() 唤醒 BLE 任务
 *   - BLE 任务被唤醒后调用 TMOS_SystemProcess()
 */
#include "ble_phy.h"
#include "CONFIG.h"
#include "HAL.h"
#include "peripheral.h"
#include "FreeRTOS.h"
#include "task.h"

/* BLE 协议栈内存，固定放在 0x20003800 */
__align(4) u32 MEM_BUF[BLE_MEMHEAP_SIZE / 4] __attribute__((at(0x20003800)));

TaskHandle_t ble_task_handle = NULL;

void ble_phy_notify_from_isr(void)
{
    BaseType_t woken = pdFALSE;

    if (ble_task_handle)
        xTaskNotifyFromISR(ble_task_handle, 0, eSetBits, &woken);
    portYIELD_FROM_ISR(woken);
}

#ifdef BLE_ENABLE
static void ble_tmos_task(void *arg)
{
    (void)arg;

    CH57X_BLEInit();
    HAL_Init();
    GAPRole_PeripheralInit();
    Peripheral_Init();

    for (;;) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        TMOS_SystemProcess();
    }
}

uint8_t ble_phy_init(void)
{
    if (ble_task_handle)
        return 0;

    xTaskCreate(ble_tmos_task, "ble", 256, NULL, 3, &ble_task_handle);
    return 0;
}
#else
uint8_t ble_phy_init(void)
{
    /* BLE_ENABLE 未开启：空实现 */
    return 0;
}
#endif
