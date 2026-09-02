/**
 * ble_phy.h —— BLE 物理层装配接口
 */
#ifndef BLE_PHY_H
#define BLE_PHY_H
#include <stdint.h>

uint8_t ble_phy_init(void);

/* 由 BLE/RTC 中断调用，唤醒 BLE 任务处理 TMOS */
void ble_phy_notify_from_isr(void);

#endif /* BLE_PHY_H */
