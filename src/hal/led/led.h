/**
 * led.h —— 板载/外接 LED 驱动接口
 *
 * 目前只提供“运行指示 LED”：
 *   - halLedInit      初始化 GPIO
 *   - halLedRunOn     点亮
 *   - halLedRunOff    熄灭
 *   - halLedRunBlink  翻转
 */
#ifndef LED_H
#define LED_H

#ifdef __cplusplus
extern "C" {
#endif

void halLedInit(void);
void halLedRunOn(void);
void halLedRunOff(void);
void halLedRunBlink(void);

#ifdef __cplusplus
}
#endif

#endif /* LED_H */
