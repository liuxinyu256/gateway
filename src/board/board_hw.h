/**
 * board_hw.h —— 板级硬件抽象接口
 *
 * 负责隔离“外围电路差异”：
 *   - 485 电路使能
 *   - 品牌电路选择（美的/东芝/海尔等）
 *   - 板上电阻/多联机电路开关
 *
 * 与 decoder/encoder 同风格：基类 ops + 具体平台实现。
 * 业务层只依赖 board_hw_t，不感知具体引脚。
 */
#ifndef BOARD_HW_H
#define BOARD_HW_H
#include <stdint.h>

typedef struct board_hw board_hw_t;

typedef struct board_hw_ops {
    uint8_t (*init)(board_hw_t *hw, const void *cfg);
    void    (*rs485_enable)(board_hw_t *hw, uint8_t enable); /* 1=打开, 0=关闭 */
} board_hw_ops_t;

struct board_hw {
    const board_hw_ops_t *ops;
    void                 *drv;
};

uint8_t board_hw_init(board_hw_t *hw, const void *cfg);
void    board_hw_rs485_enable(board_hw_t *hw, uint8_t enable);

#endif /* BOARD_HW_H */
