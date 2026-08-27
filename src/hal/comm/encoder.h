#ifndef ENCODER_H
#define ENCODER_H
#include <stdint.h>

typedef struct encoder encoder_t;

typedef struct encoder_ops {
    uint8_t (*configure)   (encoder_t *e, const void *cfg);
    uint8_t (*encode_byte) (encoder_t *e, uint8_t byte);   /* 输入字节，输出bit流 */
    void    (*tx_enable)   (encoder_t *e);
    void    (*tx_disable)  (encoder_t *e);
    uint8_t (*tx_ready)    (encoder_t *e);                 /* 可以写下一个字节 */
    uint8_t (*tx_complete) (encoder_t *e);                 /* 最后一位已到总线 */
} encoder_ops_t;

struct encoder {
    const encoder_ops_t *ops;
    void                *drv;
};

uint8_t encoder_configure(encoder_t *e, const void *cfg);
uint8_t encoder_encode_byte(encoder_t *e, uint8_t byte);
void    encoder_tx_enable(encoder_t *e);
void    encoder_tx_disable(encoder_t *e);
uint8_t encoder_tx_ready(encoder_t *e);
uint8_t encoder_tx_complete(encoder_t *e);

#endif
