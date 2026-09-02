/**
 * ble_phy.c —— BLE 物理层装配
 *
 * 接入 CH57xBLE 协议栈：
 *   - BLE 初始化放到 FreeRTOS 任务里，避免在调度器启动前卡死
 *   - 初始化完成后循环 TMOS_SystemProcess()
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

/* BLE 初始化 + TMOS 调度任务 */
static void ble_tmos_task(void *arg)
{
    (void)arg;

    CH57X_BLEInit();
    HAL_Init();
    GAPRole_PeripheralInit();
    Peripheral_Init();

    for (;;) {
        TMOS_SystemProcess();
        vTaskDelay(pdMS_TO_TICKS(1));   /* 让出 CPU，避免饿死业务任务 */
    }
}

uint8_t ble_phy_init(void)
{
    /* 只创建任务，真正初始化等调度器启动后再做 */
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
