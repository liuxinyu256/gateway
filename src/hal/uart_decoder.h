#ifndef UART_DECODER_H
#define UART_DECODER_H
#include "decoder.h"
#include "uart.h"
#include "receiver.h"

/**
 * uart_decoder —— UART 解码器
 *
 * 基于通用 uart_t 物理层接口：
 * UART 中断读到字节后，通过 decoder_feed_byte() 喂给解码器，
 * 解码器再把字节通过回调交给上层（通常是 receiver_put_byte）。
 */

typedef struct {
    decoder_t base;
    uart_t   *port;      /* 底层 UART 口 */
} uart_decoder_t;

typedef struct {
    uart_t     *port;       /* 底层 UART 口 */
    uart_cfg_t  uart_cfg;   /* UART 配置：波特率/数据位/停止位/校验位 */
} uart_decoder_cfg_t;

int uart_decoder_init(uart_decoder_t *d, const uart_decoder_cfg_t *cfg);

/* 在 UART 统一中断回调里调用：把 UART 收到的字节喂给解码器 */
void uart_decoder_poll(uart_decoder_t *d);

/* 把接收器注入解码器：解码器收到字节后回调 receiver_put_byte */
void uart_decoder_attach_receiver(uart_decoder_t *d, receiver_t *rx);

#endif
