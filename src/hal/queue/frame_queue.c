#include "frame_queue.h"
#include <string.h>

#ifdef FAKE_FREERTOS
#define FQ_ENTER_CRITICAL()
#define FQ_EXIT_CRITICAL()
#else
#include "FreeRTOS.h"
#include "task.h"
#define FQ_ENTER_CRITICAL() taskENTER_CRITICAL()
#define FQ_EXIT_CRITICAL()  taskEXIT_CRITICAL()
#endif

void frame_queue_init(frame_queue_t *q)
{
    if (!q) return;
    memset(q, 0, sizeof(*q));
}

uint8_t frame_queue_push(frame_queue_t *q,
                         const uint8_t *data, uint16_t len)
{
    if (!q || !data || len == 0 || len > TX_FRAME_MAX)
        return 1;

    FQ_ENTER_CRITICAL();

    if (q->count >= TX_FRAME_QUEUE_LEN) {
        q->drop_cnt++;            /* 队列满: 整帧丢弃，不写半帧 */
        FQ_EXIT_CRITICAL();
        return 1;
    }

    tx_frame_t *slot = &q->pool[q->tail];
    memcpy(slot->data, data, len);
    slot->len = len;

    q->tail = (uint8_t)((q->tail + 1) % TX_FRAME_QUEUE_LEN);
    q->count++;

    FQ_EXIT_CRITICAL();
    return 0;
}

uint8_t frame_queue_pop(frame_queue_t *q, tx_frame_t *out)
{
    if (!q || !out)
        return 1;

    FQ_ENTER_CRITICAL();

    if (q->count == 0) {
        FQ_EXIT_CRITICAL();
        return 1;
    }

    *out = q->pool[q->head];

    q->head = (uint8_t)((q->head + 1) % TX_FRAME_QUEUE_LEN);
    q->count--;

    FQ_EXIT_CRITICAL();
    return 0;
}

uint8_t frame_queue_empty(const frame_queue_t *q)
{
    return (!q || q->count == 0) ? 1 : 0;
}

uint16_t frame_queue_drop_count(const frame_queue_t *q)
{
    return q ? q->drop_cnt : 0;
}
