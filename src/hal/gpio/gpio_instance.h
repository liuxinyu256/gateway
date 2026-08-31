/**
 * gpio_instance.h —— GPIO 全局实例
 *
 * 与 uart_instance / timer_instance 同样式：
 *   - 提供 gpio_get_instance(port, pin) 获取全局 GPIO 对象
 *   - CH579: port 0=GPIOA, 1=GPIOB
 *   - 使用前仍需 gpio_init() 完成引脚配置
 */
#ifndef GPIO_INSTANCE_H
#define GPIO_INSTANCE_H
#include "gpio.h"

#define GPIO_PORT_A 0
#define GPIO_PORT_B 1

#define GPIO_PIN_COUNT_A 16
#define GPIO_PIN_COUNT_B 24

gpio_t *gpio_get_instance(uint8_t port, uint8_t pin);

#endif /* GPIO_INSTANCE_H */
