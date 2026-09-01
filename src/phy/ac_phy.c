/**
 * ac_phy.c —— AC 物理层装配：通用分发
 *
 * 根据品牌 phy_cfg 的 phy_type 分发到具体物理层实现。
 */
#include "ac_phy.h"

uint8_t ac_phy_setup(const ac_phy_cfg_t *phy, bus_t *bus, ac_io_t *io)
{
    if (!phy || !bus || !io)
        return 1;

    switch (phy->phy_type) {
    case AC_PHY_UART:
        return ac_phy_uart_setup((const uart_phy_cfg_t *)phy->cfg, bus, io);
    default:
        return 1;
    }
}
