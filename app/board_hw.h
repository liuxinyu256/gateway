/**
 * board_hw.h —— 板级硬件初始化
 *
 * 负责 AC 模块相关的外围电路选择：
 *   - 关闭/打开多联机通讯电路
 *   - 选择空调品牌电路（海尔/美的/东芝）
 *   - 打开 485 电路
 */
#ifndef BOARD_HW_H
#define BOARD_HW_H

void board_hw_init(void);

#endif /* BOARD_HW_H */
