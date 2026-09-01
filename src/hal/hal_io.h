/**
 * hal_io.h —— HAL IO 统一聚合头
 *
 * 把编码器/解码器/UART 实例/定时器实例这些 IO 相关接口聚合在一起，
 * 模块只需包含本头，不必关心具体是哪种物理实现。
 */
#ifndef HAL_IO_H
#define HAL_IO_H

#include "encoder.h"
#include "decoder.h"
#include "uart_encoder.h"
#include "uart_decoder.h"
#include "uart_instance.h"
#include "timer_instance.h"

#endif /* HAL_IO_H */
