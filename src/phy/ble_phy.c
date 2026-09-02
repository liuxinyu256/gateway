/**
 * ble_phy.c —— BLE 物理层骨架
 *
 * TODO:
 *   1. 加入 CH57xBLE.lib / CH57xBLE_LIB.H
 *   2. 定义 MEM_BUF 到 0x20003800
 *   3. 调用 CH57X_BLEInit / HAL_Init / GAPRole_PeripheralInit
 *   4. 创建 FreeRTOS 任务循环 TMOS_SystemProcess()
 */
#include "ble_phy.h"

uint8_t ble_phy_init(void)
{
    /* TODO: CH57xBLE 初始化 + TMOS 任务 */
    return 0;
}
