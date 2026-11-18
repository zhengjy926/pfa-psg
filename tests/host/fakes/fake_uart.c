#include "fakes/fake_uart.h"

#include <string.h>

#include "drv/uart/uart_ring.h"

static size_t fake_uart_read(void *context, uart_rx_byte_t *out, size_t capacity)
{
    fake_uart_t *fake = (fake_uart_t *)context;

    return uart_ring_pop(&fake->data, out, capacity);
}

static uint32_t fake_uart_take_gaps(void *context)
{
    fake_uart_t *fake = (fake_uart_t *)context;

    return uart_ring_take_gaps(&fake->data);
}

static bool fake_uart_write(void *context, const uint8_t *frame, size_t length)
{
    fake_uart_t *fake = (fake_uart_t *)context;
    uint8_t slot;

    if (uart_tx_is_busy(&fake->data) || (length > FAKE_UART_FRAME_BYTES)) {
        return false;
    }
    if (fake->frame_count >= FAKE_UART_TX_QUEUE) {
        return false;
    }

    slot = (uint8_t)((fake->frame_head + fake->frame_count) % FAKE_UART_TX_QUEUE);
    (void)memcpy(fake->frames[slot], frame, length);
    fake->lengths[slot] = length;
    fake->frame_count++;
    fake->frames_written++;

    uart_tx_begin(&fake->data, frame, (uint32_t)length);
    fake->tx_start_us = fake->clock->now_us;
    fake->tx_duration_us = psg_uart_frame_duration_us((uint32_t)length);
    return true;
}

static bool fake_uart_poll_complete(void *context, uint64_t *completion_us)
{
    fake_uart_t *fake = (fake_uart_t *)context;

    if (uart_tx_is_busy(&fake->data)) {
        uint64_t done_at = fake->tx_start_us + fake->tx_duration_us;
        if (psg_time_reached(fake->clock->now_us, done_at)) {
            uart_tx_mark_complete(&fake->data, done_at);
        }
    }
    return uart_tx_poll_complete(&fake->data, completion_us);
}

static bool fake_uart_tx_busy(const void *context)
{
    const fake_uart_t *fake = (const fake_uart_t *)context;

    return uart_tx_is_busy(&fake->data);
}

static const uart_ops_t g_uart_ops = {
    .read = fake_uart_read,
    .take_gaps = fake_uart_take_gaps,
    .write = fake_uart_write,
    .poll_tx_complete = fake_uart_poll_complete,
    .tx_busy = fake_uart_tx_busy,
};

void fake_uart_init(fake_uart_t *fake, const fake_timebase_t *clock)
{
    (void)memset(fake, 0, sizeof(*fake));
    fake->config.name = "fake-uart";
    fake->config.instance = 0u;
    fake->config.irq_priority = 5u;
    fake->config.dma_rx = false;
    fake->clock = clock;
    fake->base.config = &fake->config;
    fake->base.data = &fake->data;
    fake->base.context = fake;
    fake->base.ops = &g_uart_ops;
}

void fake_uart_push_byte(fake_uart_t *fake, uint8_t value, uint32_t timestamp_us)
{
    uart_ring_push(&fake->data, value, timestamp_us);
}

void fake_uart_push_bytes(fake_uart_t *fake, const uint8_t *bytes, size_t count, uint32_t timestamp_us)
{
    size_t index;

    for (index = 0u; index < count; index++) {
        fake_uart_push_byte(fake, bytes[index], timestamp_us);
    }
}

bool fake_uart_take_tx(fake_uart_t *fake, uint8_t *out, size_t capacity, size_t *length)
{
    size_t frame_length;

    if (fake->frame_count == 0u) {
        return false;
    }
    frame_length = fake->lengths[fake->frame_head];
    if (frame_length > capacity) {
        return false;
    }
    (void)memcpy(out, fake->frames[fake->frame_head], frame_length);
    if (length != NULL) {
        *length = frame_length;
    }
    fake->frame_head = (uint8_t)((fake->frame_head + 1u) % FAKE_UART_TX_QUEUE);
    fake->frame_count--;
    return true;
}

bool fake_uart_tx_empty(const fake_uart_t *fake)
{
    return fake->frame_count == 0u;
}
