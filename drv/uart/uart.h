/*
 * UART device interface (I-001, SRS N-003).
 *
 * Receive is a zero-copy ring fed by the port's DMA/IDLE interrupt.  Every
 * byte carries the time it was received so ECSP half-packet timing never
 * depends on when the task runs (T-011).  If the ring overflows the oldest
 * bytes are dropped, the gap counter is incremented, and ECSP resynchronises
 * (HW-03); the timestamps of surviving bytes are unchanged.
 *
 * Transmit is non-blocking with one frame in flight.  The buffer handed to
 * write() must stay valid until the completion poll reports the frame done.
 */
#ifndef PSG_DRV_UART_H
#define PSG_DRV_UART_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define UART_RX_RING_BYTES 1024u

typedef struct {
    uint8_t value;
    uint32_t timestamp_us;
} uart_rx_byte_t;

typedef struct {
    /* Parallel arrays keep the byte stream contiguous and the RAM cost at
     * 5 bytes per buffered byte. */
    uint8_t ring[UART_RX_RING_BYTES];
    uint32_t stamps[UART_RX_RING_BYTES];
    volatile uint32_t head;
    volatile uint32_t tail;
    volatile uint32_t gaps;
    const uint8_t *tx_data;
    volatile uint32_t tx_len;
    volatile bool tx_in_flight;
    volatile bool tx_complete_pending;
    volatile uint64_t tx_complete_us;
    bool initialized;
} uart_data_t;

typedef struct uart uart_t;

typedef struct {
    const char *name;
    uint32_t instance;
    uint8_t irq_priority;
    bool dma_rx;
} uart_config_t;

typedef struct {
    size_t (*read)(void *context, uint8_t *values, uint32_t *timestamps, size_t capacity);
    uint32_t (*take_gaps)(void *context);
    bool (*write)(void *context, const uint8_t *frame, size_t length);
    bool (*poll_tx_complete)(void *context, uint64_t *completion_us);
    bool (*tx_busy)(const void *context);
} uart_ops_t;

struct uart {
    const uart_config_t *config;
    uart_data_t *data;
    void *context;
    const uart_ops_t *ops;
};

static inline size_t uart_read(const uart_t *uart, uint8_t *values, uint32_t *timestamps, size_t capacity)
{
    return uart->ops->read(uart->context, values, timestamps, capacity);
}

static inline uint32_t uart_take_gaps(const uart_t *uart)
{
    return uart->ops->take_gaps(uart->context);
}

static inline bool uart_write(const uart_t *uart, const uint8_t *frame, size_t length)
{
    return uart->ops->write(uart->context, frame, length);
}

static inline bool uart_poll_tx_complete(const uart_t *uart, uint64_t *completion_us)
{
    return uart->ops->poll_tx_complete(uart->context, completion_us);
}

static inline bool uart_tx_busy(const uart_t *uart)
{
    return uart->ops->tx_busy(uart->context);
}

#endif /* PSG_DRV_UART_H */
