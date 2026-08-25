#ifndef UART_ENCODER_H
#define UART_ENCODER_H
#include "encoder.h"
#include "uart.h"

typedef struct {
    encoder_t   base;
    uart_t     *port;
    uart_cfg_t  uart_cfg;
} uart_encoder_t;

typedef struct {
    uart_t     *port;
    uart_cfg_t  uart_cfg;
} uart_encoder_cfg_t;

uint8_t uart_encoder_init(uart_encoder_t *e,
                          const uart_encoder_cfg_t *cfg);

#endif
