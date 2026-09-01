/**
 * ac_phy.h —— AC 物理层抽象
 *
 * 每个物理层是一个“类”，提供创建 IO 对象的方法（create_io）。
 * 上层通过 ac_phy_ops_t 调用，不感知具体物理实现。
 */
#ifndef AC_PHY_H
#define AC_PHY_H
#include "ac_module.h"
#include "encoder.h"
#include "decoder.h"
#include "sender.h"
#include "receiver.h"
#include "bus.h"
#include "rs485.h"

typedef struct {
    encoder_t  *encoder;
    decoder_t  *decoder;
    sender_t   *sender;
    receiver_t *receiver;
    rs485_t    *rs485;   /* RS485 方向控制（若该物理层是 485） */
} ac_io_t;

/* 物理层类接口：每个物理层实现 create_io */
typedef struct ac_phy_ops {
    uint8_t (*create_io)(const void *cfg, bus_t *bus, ac_io_t *io);
} ac_phy_ops_t;

/* 内置物理层类 */
extern const ac_phy_ops_t ac_phy_uart_ops;

/* 根据品牌 phy_cfg 装配（分发到具体物理层类） */
uint8_t ac_phy_init(const ac_phy_cfg_t *phy, bus_t *bus, ac_io_t *io);

#endif /* AC_PHY_H */
