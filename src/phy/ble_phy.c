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
#include "ble_proto_1to1.h"
#include "gateway.h"
#include "debug_module.h"
#include "FreeRTOS.h"
#include "task.h"

/* BLE 协议栈内存，固定放在 0x20003800 */
__align(4) u32 MEM_BUF[BLE_MEMHEAP_SIZE / 4] __attribute__((at(0x20003800)));

TaskHandle_t ble_task_handle = NULL;

void ble_phy_notify_from_isr(void)
{
    BaseType_t woken = pdFALSE;

    if (ble_task_handle)
        xTaskNotifyFromISR(ble_task_handle, 1, eSetBits, &woken);
    portYIELD_FROM_ISR(woken);
}

#ifdef BLE_ENABLE
/* 模块状态变化 -> BLE 通知上位机 */
static void ble_state_change_cb(uint8_t module_id, const gateway_state_t *s, void *ctx)
{
    (void)ctx;
    if (module_id == BLE1TO1_MODULE_ID)
        ble_peripheral_notify_state(s);
}

static void ble_tmos_task(void *arg)
{
    (void)arg;

    log_printf("[ble] task start\r\n");
    CH57X_BLEInit();
    log_printf("[ble] ble init ok\r\n");
    HAL_Init();
    log_printf("[ble] hal init ok\r\n");
    GAPRole_PeripheralInit();
    log_printf("[ble] gap init ok\r\n");
    Peripheral_Init();
    gateway_on_state_change(ble_state_change_cb, NULL);
    log_printf("[ble] peri init ok\r\n");

    for (;;) {
        /* 事件通知唤醒 + 1ms 超时兜底：保证 TMOS 持续被调度 */
        ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(1));
        TMOS_SystemProcess();
    }
}

uint8_t ble_phy_init(void)
{
    if (ble_task_handle)
        return 0;

    xTaskCreate(ble_tmos_task, "ble", 256, NULL, 2, &ble_task_handle);
    return 0;
}
#else
uint8_t ble_phy_init(void)
{
    /* BLE_ENABLE 未开启：空实现 */
    return 0;
}
#endif
