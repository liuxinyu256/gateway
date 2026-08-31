/**
 * bsp_a07s.h —— A07S 板级硬件具体实现
 *
 * A07S 不是美的/东芝/海尔专用适配板，不涉及品牌选择电路，
 * 只配置 485 电路使能引脚。
 */
#ifndef BSP_A07S_H
#define BSP_A07S_H
#include "bsp.h"
#include "gpio.h"

typedef struct {
    bsp_t base;

    /* A07S 只使用 485 电路控制引脚 */
    gpio_t *pb5;   /* 485 电路 */
    gpio_t *pb6;   /* 485 电路 */
} bsp_a07s_t;

typedef struct {
    uint8_t reserved; /* 预留，后续可放默认品牌等 */
} bsp_a07s_cfg_t;

extern const bsp_ops_t bsp_a07s_ops;

uint8_t bsp_a07s_init(bsp_a07s_t *self,
                       const bsp_a07s_cfg_t *cfg);

/* 板级选择接口：初始化/获取本板实例 */
uint8_t bsp_a07s_board_init(void);
bsp_t  *bsp_a07s_board_get(void);

#endif /* BSP_A07S_H */
