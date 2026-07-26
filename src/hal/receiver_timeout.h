#ifndef RECEIVER_TIMEOUT_H
#define RECEIVER_TIMEOUT_H
#include "receiver.h"
#include "frame_timer.h"
receiver_t* receiver_timeout_create(frame_timer_t *timer, uint16_t timeout_ticks,
                                     frame_finish_callback cb,
                                     uint8_t *ring_buf, uint16_t ring_size);
void receiver_timeout_destroy(receiver_t *pkt);
#endif
