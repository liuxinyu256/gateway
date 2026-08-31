/**
 * rs485.c —— RS485 方向控制通用分发
 */
#include "rs485.h"

uint8_t rs485_init(rs485_t *rs, const void *cfg)
{
    if (!rs || !rs->ops || !rs->ops->init)
        return 1;
    return rs->ops->init(rs, cfg);
}

void rs485_set_dir(rs485_t *rs, uint8_t tx)
{
    if (!rs || !rs->ops || !rs->ops->set_dir)
        return;
    rs->ops->set_dir(rs, tx);
}
