/**
 * led_ch579.c —— CH579 LED 驱动
 *
 * 运行 LED 使用 PA0（低电平点亮）。
 * 注意：PA0 原先是 RS485 DE，为避免冲突，RS485 DE 已改到 PA1。
 * 引脚控制统一走 GPIO HAL。
 */
#include "led.h"

#ifdef __CH579__
#include "gpio.h"
#include "gpio_instance.h"

#define RUN_LED_PORT  GPIO_PORT_A      /* 0=GPIOA */
#define RUN_LED_PIN   0                /* PA0 引脚索引 */
#define RUN_LED_MASK  (1u << RUN_LED_PIN) /* PA0 掩码 */

static gpio_t *s_run_led;

void halLedInit(void)
{
    gpio_cfg_t cfg = {
        .port       = RUN_LED_PORT,
        .pin        = RUN_LED_MASK,
        .mode       = GPIO_MODE_OUTPUT_PP,
        .init_level = GPIO_LEVEL_LOW,   /* 默认点亮 */
    };

    s_run_led = gpio_get(RUN_LED_PORT, RUN_LED_PIN);
    if (s_run_led)
        gpio_init(s_run_led, &cfg);
}

void halLedRunOn(void)
{
    if (s_run_led)
        gpio_set(s_run_led, GPIO_LEVEL_LOW);   /* 低电平点亮 */
}

void halLedRunOff(void)
{
    if (s_run_led)
        gpio_set(s_run_led, GPIO_LEVEL_HIGH);  /* 高电平熄灭 */
}

void halLedRunBlink(void)
{
    if (s_run_led)
        gpio_toggle(s_run_led);
}

#endif /* __CH579__ */
