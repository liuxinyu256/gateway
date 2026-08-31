/**
 * bsp_a07s.h —— CH579 板级硬件具体实现
 *
 * 当前硬件：
 *   - 默认选择美的电路
 *   - 打开 485 电路
 *   - 关闭海尔多联机/东芝/120 电阻等
 */
#ifndef BSP_A07S_H
#define BSP_A07S_H
#include "bsp.h"
#include "gpio.h"

typedef struct {
    bsp_t base;

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
