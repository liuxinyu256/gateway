/**
 * ac_phy.h —— AC 物理层装配接口
 *
 * 根据品牌 phy_cfg 创建/配置编码器、解码器、发送器、接收器，
 * 结果通过 ac_io_t 返回抽象指针，模块不感知具体物理实现。
 */
#ifndef AC_PHY_H
#define AC_PHY_H
#include "ac_module.h"
#include "encoder.h"
#include "decoder.h"
#include "sender.h"
#include "receiver.h"
#include "bus.h"

typedef struct {
    encoder_t  *encoder;   /* 编码器抽象 */
    decoder_t  *decoder;   /* 解码器抽象 */
    sender_t   *sender;    /* 发送器抽象 */
    receiver_t *receiver;  /* 接收器抽象 */
} ac_io_t;

uint8_t ac_phy_setup(const ac_phy_cfg_t *phy, bus_t *bus, ac_io_t *io);

/* 各物理层实现 */
uint8_t ac_phy_uart_setup(const uart_phy_cfg_t *cfg, bus_t *bus, ac_io_t *io);

#endif /* AC_PHY_H */
