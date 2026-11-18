#include "drv/uart/uart_ring.h"

#define UART_RX_RING_MASK (UART_RX_RING_BYTES - 1u)

_Static_assert((UART_RX_RING_BYTES & UART_RX_RING_MASK) == 0u, "ring size must be a power of two");

void uart_ring_push(uart_data_t *data, uint8_t value, uint32_t timestamp_us)
{
    uint32_t head = data->head;
    uint32_t next = (head + 1u) & UART_RX_RING_MASK;

    if (next == data->tail) {
        /* Full: drop the oldest byte, count the gap, keep the newest byte and
         * its timestamp (HW-03). */
        data->tail = (data->tail + 1u) & UART_RX_RING_MASK;
        data->gaps++;
    }

    data->ring[head] = value;
    data->stamps[head] = timestamp_us;
    data->head = next;
}

size_t uart_ring_pop(uart_data_t *data, uint8_t *values, uint32_t *timestamps, size_t capacity)
{
    size_t count = 0u;

    while ((count < capacity) && (data->tail != data->head)) {
        values[count] = data->ring[data->tail];
        timestamps[count] = data->stamps[data->tail];
        data->tail = (data->tail + 1u) & UART_RX_RING_MASK;
        count++;
    }
    return count;
}

uint32_t uart_ring_take_gaps(uart_data_t *data)
{
    uint32_t gaps = data->gaps;
    data->gaps = 0u;
    return gaps;
}

void uart_tx_begin(uart_data_t *data, const uint8_t *frame, uint32_t length)
{
    data->tx_data = frame;
    data->tx_len = length;
    data->tx_complete_pending = false;
    data->tx_in_flight = true;
}

void uart_tx_mark_complete(uart_data_t *data, uint64_t completion_us)
{
    data->tx_in_flight = false;
    data->tx_complete_us = completion_us;
    data->tx_complete_pending = true;
}

bool uart_tx_poll_complete(uart_data_t *data, uint64_t *completion_us)
{
    if (!data->tx_complete_pending) {
        return false;
    }
    data->tx_complete_pending = false;
    if (completion_us != NULL) {
        *completion_us = data->tx_complete_us;
    }
    return true;
}

bool uart_tx_is_busy(const uart_data_t *data)
{
    return data->tx_in_flight;
}
