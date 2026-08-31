/**
 * led_ch579.c —— CH579 LED 驱动
 *
 * 运行 LED 使用 PA0（低电平点亮）。
 * 注意：PA0 原先是 RS485 DE，为避免冲突，RS485 DE 已改到 PA1。
 */
#include "led.h"

#ifdef __CH579__
#include "CH57x_common.h"

#define PIN_RUN_LED  GPIO_Pin_0  /* 空调运转灯：低电平点亮 */

void halLedInit(void)
{
    GPIOA_ResetBits(PIN_RUN_LED);
    GPIOA_ModeCfg(PIN_RUN_LED, GPIO_ModeOut_PP_5mA);
}

void halLedRunOn(void)
{
    GPIOA_ResetBits(PIN_RUN_LED);
    GPIOA_ModeCfg(PIN_RUN_LED, GPIO_ModeOut_PP_5mA);
}

void halLedRunOff(void)
{
    GPIOA_SetBits(PIN_RUN_LED);
    GPIOA_ModeCfg(PIN_RUN_LED, GPIO_ModeOut_PP_5mA);
}

void halLedRunBlink(void)
{
    GPIOA_InverseBits(PIN_RUN_LED);
    GPIOA_ModeCfg(PIN_RUN_LED, GPIO_ModeOut_PP_5mA);
}

#endif /* __CH579__ */
