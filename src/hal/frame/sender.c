/**
 * sender.c —— 帧级发送器（帧任务 + 双环形缓冲）
 *
 * 生产者：send_task 只往 sender_send() 投递帧
 * sender_send()：
 *   - 按优先级把字节写入 cmd_ring / norm_ring
 *   - 同时把帧任务（len）放入 cmd_jobs / norm_jobs
 *
 * 消费者：sender_pump() 优先取 cmd_jobs，再取 norm_jobs
 * 字节搬移：UART ISR / 定时器 tick -> sender_isr() -> encoder
 * 帧间 gap：tx_done -> gap timer -> EVENT_BUS_IDLE -> sender_pump()
 */
#include "sender.h"
#include <string.h>

#ifdef FAKE_FREERTOS
#define S_ENTER_CRITICAL()
#define S_EXIT_CRITICAL()
#else
#include "FreeRTOS.h"
#include "task.h"
#define S_ENTER_CRITICAL() taskENTER_CRITICAL()
#define S_EXIT_CRITICAL()  taskEXIT_CRITICAL()
#endif

/* ---- 帧任务 FIFO ---- */
static uint8_t job_queue_push(tx_job_queue_t *q, uint16_t len)
{
    if (!q || q->count >= TX_JOB_QUEUE_LEN)
        return 1;

    q->jobs[q->tail].len  = len;
    q->jobs[q->tail].sent = 0;
    q->tail = (uint8_t)((q->tail + 1) % TX_JOB_QUEUE_LEN);
    q->count++;
    return 0;
}

static uint8_t job_queue_pop(tx_job_queue_t *q, tx_job_t *out)
{
    if (!q || q->count == 0 || !out)
        return 1;

    *out = q->jobs[q->head];
    q->head = (uint8_t)((q->head + 1) % TX_JOB_QUEUE_LEN);
    q->count--;
    return 0;
}

static uint16_t ring_free(const ring_t *r)
{
    if (!r || r->size == 0)
        return 0;
    /* ring 为 2 的幂，满/空用 size-1 容量 */
    return r->size - 1 - ring_count(r);
}

uint8_t sender_init(sender_t *tx, const sender_cfg_t *cfg)
{
    if (!tx || !cfg || !cfg->encoder || !cfg->bus ||
        !cfg->cmd_ring_buf || cfg->cmd_ring_size < 4 ||
        !cfg->norm_ring_buf || cfg->norm_ring_size < 4)
        return 1;

    /* ring 内部用 & mask 实现回绕，必须是 2 的幂 */
    if ((cfg->cmd_ring_size & (cfg->cmd_ring_size - 1)) != 0 ||
        (cfg->norm_ring_size & (cfg->norm_ring_size - 1)) != 0)
        return 1;

    memset(tx, 0, sizeof(*tx));
    tx->encoder = cfg->encoder;
    tx->bus     = cfg->bus;

    ring_init(&tx->cmd_ring, cfg->cmd_ring_buf, cfg->cmd_ring_size);
    ring_init(&tx->norm_ring, cfg->norm_ring_buf, cfg->norm_ring_size);
    return 0;
}

uint8_t sender_send(sender_t *tx,
                    const uint8_t *frame, uint16_t len,
                    uint8_t priority)
{
    ring_t        *ring;
    tx_job_queue_t *q;

    if (!tx || !frame || !len)
        return 1;

    ring = priority ? &tx->cmd_ring : &tx->norm_ring;
    q    = priority ? &tx->cmd_jobs : &tx->norm_jobs;

    if (ring_free(ring) < len || q->count >= TX_JOB_QUEUE_LEN) {
        tx->drop_cnt++;
        return 1;   /* ring 或任务队列满：本次不入队，由调用方决定 */
    }

    if (ring_write(ring, frame, len) != len) {
        /* 理论上不会发生，上面已经检查过空间 */
        return 1;
    }

    /* q->count 已在上面检查，任务入队不会再失败 */
    (void)job_queue_push(q, len);

    sender_pump(tx);
    return 0;
}

void sender_pump(sender_t *tx)
{
    if (!tx)
        return;

    if (tx->sending)
        return;

    if (!bus_is_idle(tx->bus))
        return;

    tx_job_t job;
    uint8_t  prio;

    /* CMD 优先 */
    if (job_queue_pop(&tx->cmd_jobs, &job) == 0) {
        prio = 1;
    } else if (job_queue_pop(&tx->norm_jobs, &job) == 0) {
        prio = 0;
    } else {
        return;
    }

    tx->current          = job;
    tx->current_prio     = prio;
    tx->wait_tx_complete = 0;
    tx->sending          = 1;

    bus_mark_busy(tx->bus);           /* bus 负责方向控制: 进入发送方向 */
    encoder_tx_enable(tx->encoder);   /* 只开中断/定时器，ISR/tick 自己取字节 */
}

/* 一帧数据已经全部写入 THR，不需要等 TX_COMPLETE 的情况 */
static void finish_frame_no_tx_complete(sender_t *tx)
{
    if (!tx)
        return;

    tx->sending = 0;
    tx->wait_tx_complete = 0;

    if (tx->bus)
        bus_on_thr_empty(tx->bus);    /* 不需要等 TX 完成：直接进入 gap */

    if (tx->on_done)
        tx->on_done(tx);              /* tx_done */
}

static void on_thr_empty(sender_t *tx)
{
    ring_t *ring;

    if (!tx || !tx->sending || !tx->encoder)
        return;

    ring = tx->current_prio ? &tx->cmd_ring : &tx->norm_ring;

    if (tx->current.sent < tx->current.len) {
        uint8_t byte;
        if (ring_get(ring, &byte) != 0)
            return;                     /* 理论不会发生 */

        encoder_encode_byte(tx->encoder, byte);
        tx->current.sent++;

        if (tx->current.sent >= tx->current.len) {
            /* 当前帧的最后一个字节已经写进 THR */
            encoder_tx_disable(tx->encoder);

            if (tx->bus && tx->bus->need_tx_complete) {
                /* RS485：最后一位还在移位寄存器，不能换向 */
                tx->wait_tx_complete = 1;
                if (tx->ops && tx->ops->start)
                    tx->ops->start(tx);
            } else {
                finish_frame_no_tx_complete(tx);
            }
        }
        return;
    }
}

static void on_tx_complete(sender_t *tx)
{
    if (!tx || !tx->wait_tx_complete)
        return;

    tx->wait_tx_complete = 0;
    tx->sending = 0;

    if (tx->ops && tx->ops->stop)
        tx->ops->stop(tx);

    if (tx->bus)
        bus_on_tx_complete(tx->bus);  /* 真正发完，进入 gap 并释放方向 */

    if (tx->on_done)
        tx->on_done(tx);              /* tx_done */
}

uint8_t sender_poll_tx_complete(sender_t *tx)
{
    if (!tx || !tx->wait_tx_complete)
        return 0;

    if (encoder_tx_complete(tx->encoder)) {
        on_tx_complete(tx);
        return 0;
    }
    return 1;
}

void sender_tx_complete_isr(sender_t *tx)
{
    if (!tx || !tx->wait_tx_complete)
        return;

    if (encoder_tx_complete(tx->encoder))
        on_tx_complete(tx);
}

void sender_isr(sender_t *tx)
{
    if (!tx || !tx->sending)
        return;

    if (encoder_tx_ready(tx->encoder))
        on_thr_empty(tx);

    if (tx->wait_tx_complete &&
        encoder_tx_complete(tx->encoder))
        on_tx_complete(tx);
}

void sender_set_callbacks(sender_t *tx, const sender_callbacks_t *cb)
{
    if (!tx || !cb) return;

    tx->on_done = cb->done;
}

uint16_t sender_drop_count(const sender_t *tx)
{
    return tx ? tx->drop_cnt : 0;
}
