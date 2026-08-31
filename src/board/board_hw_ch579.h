/**
 * board_hw_ch579.h —— CH579 板级硬件具体实现
 *
 * 当前硬件：
 *   - 默认选择美的电路
 *   - 打开 485 电路
 *   - 关闭海尔多联机/东芝/120 电阻等
 */
#ifndef BOARD_HW_CH579_H
#define BOARD_HW_CH579_H
#include "board_hw.h"
#include "gpio.h"

typedef struct {
    board_hw_t base;

    /* 外围电路控制引脚（init 时获取） */
    gpio_t *pb5;   /* 485 电路 */
    gpio_t *pb6;   /* 485 电路 */
    gpio_t *pb8;   /* 海尔多联机通讯电路 */
    gpio_t *pb1;   /* 120 电阻 */
    gpio_t *pb9;   /* 东芝/美的电源选择 */
    gpio_t *pa14;  /* 接收口选择 */
    gpio_t *pa15;  /* 接收口选择 */
    gpio_t *pb11;  /* 浮空输入 */
    gpio_t *pb21;  /* 浮空输入 */
} board_hw_ch579_t;

typedef struct {
    uint8_t reserved; /* 预留，后续可放默认品牌等 */
} board_hw_ch579_cfg_t;

extern const board_hw_ops_t board_hw_ch579_ops;

uint8_t board_hw_ch579_init(board_hw_ch579_t *self,
                            const board_hw_ch579_cfg_t *cfg);

#endif /* BOARD_HW_CH579_H */
