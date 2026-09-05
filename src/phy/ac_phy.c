/**
 * ac_phy.c —— AC 物理层装配：通用分发
 *
 * 根据品牌 phy_cfg 的 phy_type 找到对应物理层类，调用其 create_io。
 */
#include "ac_phy.h"
#include <stddef.h>

static const ac_phy_ops_t *phy_ops(ac_phy_type_t type)
{
    switch (type) {
    /* AC_PHY_UART 暂时不参与编译以节省 RAM；需要时恢复 ac_phy_uart.c */
    case AC_PHY_RS485:    return &ac_phy_rs485_ops;
    case AC_PHY_HBS:      return NULL;   /* 预留 */
    default:              return NULL;
    }
}

uint8_t ac_phy_init(const ac_phy_cfg_t *phy, bus_t *bus, ac_io_t *io)
{
    const ac_phy_ops_t *ops;

    if (!phy || !bus || !io)
        return 1;

    ops = phy_ops(phy->phy_type);
    if (!ops || !ops->create_io)
        return 1;

    return ops->create_io(phy->cfg, bus, io);
}
