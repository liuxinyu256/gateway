/**
 * bsp.h —— 板级硬件抽象接口
 *
 * 负责隔离“外围电路差异”：
 *   - 485 电路使能
 *   - 品牌电路选择（美的/东芝/海尔等）
 *   - 板上电阻/多联机电路开关
 *
 * 与 decoder/encoder 同风格：基类 ops + 具体平台实现。
 * 业务层只依赖 bsp_t，不感知具体引脚。
 */
#ifndef BSP_H
#define BSP_H
#include <stdint.h>

typedef struct bsp bsp_t;

typedef struct bsp_ops {
    uint8_t (*init)(bsp_t *hw, const void *cfg);
    void    (*rs485_enable)(bsp_t *hw, uint8_t enable); /* 1=打开, 0=关闭 */
} bsp_ops_t;

struct bsp {
    const bsp_ops_t *ops;
    void                 *drv;
};

uint8_t bsp_init(bsp_t *hw, const void *cfg);
void    bsp_rs485_enable(bsp_t *hw, uint8_t enable);

#endif /* BSP_H */
