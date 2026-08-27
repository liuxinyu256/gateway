#ifndef DECODER_H
#define DECODER_H
#include <stdint.h>

/**
 * decoder —— 解码器
 *
 * 负责把物理信号/字节输入解码成“字节流”，输出通过回调交给上层。
 *
 * 输入有两种：
 *   - feed_byte：UART 这类硬件已经给出字节
 *   - feed_sample：曼彻斯特/单总线这类需要软件采样电平
 *
 * 输出统一：
 *   - 回调 decoder_rx_cb(byte, ctx)
 *
 * 每个解码器通过 const void *cfg 接收自己的初始化参数。
 */

typedef struct decoder decoder_t;
typedef void (*decoder_rx_cb)(uint8_t byte, void *ctx);

typedef struct decoder_ops {
    int  (*init)(decoder_t *d, const void *cfg);
    void (*set_rx_callback)(decoder_t *d, decoder_rx_cb cb, void *ctx);
    void (*feed_byte)(decoder_t *d, uint8_t byte);
    void (*feed_sample)(decoder_t *d, uint8_t level, uint32_t ts_us);
} decoder_ops_t;

struct decoder {
    const decoder_ops_t *ops;
    void                *drv;      /* 具体解码器私有数据 */
    decoder_rx_cb        rx_cb;
    void                *rx_ctx;
};

int  decoder_init(decoder_t *d, const void *cfg);
void decoder_set_rx_callback(decoder_t *d, decoder_rx_cb cb, void *ctx);
void decoder_feed_byte(decoder_t *d, uint8_t byte);
void decoder_feed_sample(decoder_t *d, uint8_t level, uint32_t ts_us);

#endif
