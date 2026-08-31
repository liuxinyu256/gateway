/**
 * gpio.h —— GPIO HAL 接口
 *
 * 与 uart/timer/encoder 同风格：
 *   - gpio_t 保存 ops + drv + 通用引脚信息（port/pin/mode/init_level）
 *   - 具体平台实现继承 gpio_t，例如 gpio_ch579_t
 *   - 上层通过 gpio_init / gpio_set / gpio_read / gpio_toggle / gpio_reset / gpio_deinit 操作引脚
 */
#ifndef GPIO_H
#define GPIO_H
#include <stdint.h>

typedef struct gpio gpio_t;

/* 统一的 GPIO 模式；具体平台驱动负责映射到自己的寄存器/库 */
typedef enum {
    GPIO_MODE_INPUT = 0,        /* 高阻输入 */
    GPIO_MODE_INPUT_PULLUP,     /* 上拉输入 */
    GPIO_MODE_INPUT_PULLDOWN,   /* 下拉输入 */
    GPIO_MODE_OUTPUT_PP,        /* 推挽输出 */
    GPIO_MODE_OUTPUT_OD,        /* 开漏输出（平台不支持时 init 返回失败） */
} gpio_mode_t;

/* 统一的 GPIO 电平 */
typedef enum {
    GPIO_LEVEL_LOW = 0,
    GPIO_LEVEL_HIGH,
} gpio_level_t;

typedef struct gpio_ops {
    uint8_t (*init)(gpio_t *g, const void *cfg);
    void    (*deinit)(gpio_t *g);
    void    (*reset)(gpio_t *g);                 /* 恢复 init 时的模式/初始电平，不释放 */
    void    (*set)(gpio_t *g, gpio_level_t level);
    gpio_level_t (*read)(gpio_t *g);
    void    (*toggle)(gpio_t *g);
} gpio_ops_t;

struct gpio {
    const gpio_ops_t *ops;
    void             *drv;

    /* 通用引脚信息：init 时写入，reset 时使用 */
    uint8_t      port;       /* 端口号：平台相关（CH579: 0=GPIOA, 1=GPIOB） */
    uint32_t     pin;        /* 引脚号：平台相关（CH579: GPIO_Pin_x） */
    uint8_t      mode;       /* gpio_mode_t */
    gpio_level_t init_level; /* 初始电平 */
};

/* 平台无关的 GPIO 配置 */
typedef struct {
    uint8_t      port;       /* 端口号：平台相关（CH579: 0=GPIOA, 1=GPIOB） */
    uint32_t     pin;        /* 引脚号：平台相关（CH579: GPIO_Pin_x） */
    uint8_t      mode;       /* gpio_mode_t */
    gpio_level_t init_level; /* 初始电平 */
} gpio_cfg_t;

uint8_t gpio_init(gpio_t *g, const gpio_cfg_t *cfg);
void    gpio_deinit(gpio_t *g);
void    gpio_reset(gpio_t *g);
void    gpio_set(gpio_t *g, gpio_level_t level);
gpio_level_t gpio_read(gpio_t *g);
void    gpio_toggle(gpio_t *g);

#endif /* GPIO_H */
