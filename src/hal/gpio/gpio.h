/**
 * gpio.h —— GPIO HAL 接口
 *
 * 与 uart/timer/encoder 同风格：
 *   - gpio_t 只保存 ops + drv
 *   - 具体平台实现继承 gpio_t，例如 gpio_ch579_t
 *   - 上层通过 gpio_init / gpio_set / gpio_toggle 操作引脚
 */
#ifndef GPIO_H
#define GPIO_H
#include <stdint.h>

typedef struct gpio gpio_t;

/* 与 CH57x 库模式对应，保持平台无关 */
typedef enum {
    GPIO_MODE_IN_FLOATING = 0,
    GPIO_MODE_IN_PU,
    GPIO_MODE_IN_PD,
    GPIO_MODE_OUT_PP_5MA,
    GPIO_MODE_OUT_PP_20MA,
} gpio_mode_t;

typedef struct gpio_ops {
    uint8_t (*init)(gpio_t *g, const void *cfg);
    void    (*set)(gpio_t *g, uint8_t level);     /* 0=低, 非0=高 */
    void    (*toggle)(gpio_t *g);
} gpio_ops_t;

struct gpio {
    const gpio_ops_t *ops;
    void             *drv;
};

/* 平台无关的 GPIO 配置 */
typedef struct {
    uint8_t  port;      /* 0=GPIOA, 1=GPIOB */
    uint32_t pin;       /* GPIO_Pin_x */
    uint8_t  mode;      /* gpio_mode_t */
    uint8_t  init_level; /* 0=低, 1=高 */
} gpio_cfg_t;

uint8_t gpio_init(gpio_t *g, const gpio_cfg_t *cfg);
void    gpio_set(gpio_t *g, uint8_t level);
void    gpio_toggle(gpio_t *g);

#endif /* GPIO_H */
