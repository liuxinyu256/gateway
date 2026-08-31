/**
 * board_hw.c —— 板级硬件初始化实现
 *
 * 所有 GPIO 操作统一走 GPIO HAL。
 */
#include "board_hw.h"
#include "gpio.h"
#include "gpio_instance.h"

static void cfg_pin(uint8_t port, uint8_t pin,
                    uint8_t mode, gpio_level_t level)
{
    gpio_t *g = gpio_get(port, pin);
    gpio_cfg_t cfg;

    if (!g)
        return;

    cfg.port       = port;
    cfg.pin        = (uint32_t)(1u << pin);
    cfg.mode       = mode;
    cfg.init_level = level;
    gpio_init(g, &cfg);
}

void board_hw_init(void)
{
    /* 关闭海尔多联机通讯电路 */
    cfg_pin(GPIO_PORT_B, 8,  GPIO_MODE_OUTPUT_PP, GPIO_LEVEL_LOW);

    /* PB11/PB21 浮空输入 */
    cfg_pin(GPIO_PORT_B, 11, GPIO_MODE_INPUT,      GPIO_LEVEL_LOW);
    cfg_pin(GPIO_PORT_B, 21, GPIO_MODE_INPUT,      GPIO_LEVEL_LOW);

    /* 关闭 120 电阻 */
    cfg_pin(GPIO_PORT_B, 1,  GPIO_MODE_OUTPUT_PP, GPIO_LEVEL_LOW);

    /* PA14 浮空输入 */
    cfg_pin(GPIO_PORT_A, 14, GPIO_MODE_INPUT,      GPIO_LEVEL_LOW);

    /* 关掉东芝电路，电源先选择美的 */
    cfg_pin(GPIO_PORT_B, 9,  GPIO_MODE_OUTPUT_PP, GPIO_LEVEL_HIGH);

    /* PA15 浮空输入，接收口选择美的 */
    cfg_pin(GPIO_PORT_A, 15, GPIO_MODE_INPUT,      GPIO_LEVEL_LOW);

    /* 打开 485 电路 */
    cfg_pin(GPIO_PORT_B, 6,  GPIO_MODE_OUTPUT_PP, GPIO_LEVEL_LOW);
    cfg_pin(GPIO_PORT_B, 5,  GPIO_MODE_OUTPUT_PP, GPIO_LEVEL_HIGH);
}
