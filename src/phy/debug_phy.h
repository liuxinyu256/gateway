/**
 * debug_phy.h —— 调试/日志模块物理层装配接口
 *
 * Debug 固定使用 UART1 115200，使用重构后的 sender 发送。
 */
#ifndef DEBUG_PHY_H
#define DEBUG_PHY_H
#include "encoder.h"
#include "decoder.h"
#include "sender.h"
#include "receiver.h"
#include "bus.h"
#include <stdint.h>

typedef struct {
    encoder_t  *encoder;
    decoder_t  *decoder;
    sender_t   *sender;
    receiver_t *receiver;
} debug_io_t;

uint8_t debug_phy_init(bus_t *bus, debug_io_t *io);

#endif /* DEBUG_PHY_H */
