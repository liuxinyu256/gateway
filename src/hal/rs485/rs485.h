/**
 * rs485.h —— RS485 方向控制 HAL 接口
 *
 * 与 encoder/decoder 同风格：
 *   - rs485_t 只保存 ops + drv
 *   - 具体平台实现继承 rs485_t，例如 rs485_ch579_t
 *   - 上层只依赖 rs485_t / rs485_init / rs485_set_dir
 */
#ifndef RS485_H
#define RS485_H
#include <stdint.h>

typedef struct rs485 rs485_t;

typedef struct rs485_ops {
    uint8_t (*init)(rs485_t *rs, const void *cfg);
    void    (*set_dir)(rs485_t *rs, uint8_t tx); /* 1=发送, 0=接收 */
} rs485_ops_t;

struct rs485 {
    const rs485_ops_t *ops;
    void              *drv;
};

uint8_t rs485_init(rs485_t *rs, const void *cfg);
void    rs485_set_dir(rs485_t *rs, uint8_t tx);

#endif /* RS485_H */
