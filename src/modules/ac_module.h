#ifndef AC_MODULE_H
#define AC_MODULE_H
#include "../brands/ac_brand.h"
#include "module.h"
#include "gateway_device.h"

/* ---- AC 模块 ---- */
typedef struct
{
    module_t base;                               /* ac_module_t 自己就是 module_t 的子类 */
    const ac_brand_config_t *const *brand_table; /* 品牌注册表地址 (init 传入) */
    uint8_t brand_count;                         /* 注册表长度 */
    const ac_brand_config_t *current;            /* 当前激活/正在尝试的品牌 */
    uint8_t scan_index;                          /* 当前扫描到的品牌表下标 */
    uint8_t locked;                              /* 品牌锁定标志: 0=扫描中, 1=已锁定 */

    uint8_t rx_buf[128];                         /* AC 模块接收缓冲区 */
} ac_module_t;

/* AC 模块初始化参数 (通过 module_init 的 cfg 传入) */
typedef struct {
    uint32_t baudrate;
    const ac_brand_config_t *const *brand_table;
    uint8_t brand_count;
} ac_init_cfg_t;

extern const module_ops_t ac_module_ops;

/* brand_table/brand_count: 品牌注册表地址与长度 (编译期静态表)
 * sender / receiver 都由上层指针注入，模块不持有具体实现 */
void ac_module_init(ac_module_t *self,
                    const ac_brand_config_t *const *brand_table,
                    uint8_t brand_count);
void ac_module_register(ac_module_t *self, const ac_brand_config_t *cfg);
void ac_module_start_scan(ac_module_t *self);
void ac_module_lock(ac_module_t *self);
uint8_t ac_module_locked(ac_module_t *self);
const ac_brand_config_t *ac_module_current(ac_module_t *self);
void ac_module_set_poll_period(ac_module_t *self, uint16_t period_ms);

/* 模块级发送：AC 单例，品牌层只需传 frame + len，默认 CMD 优先级 */
uint8_t ac_module_send_frame(const uint8_t *frame, uint16_t len);

/* 状态上报：AC 模块状态变化后同步到网关 */
void ac_module_publish_state(ac_module_t *self);
/* 更新 AC 模块状态：new_state 必须是“读当前完整状态 → 改支持字段”后的完整状态 */
void ac_module_update_state(ac_module_t *self, const gateway_state_t *new_state);
#endif
