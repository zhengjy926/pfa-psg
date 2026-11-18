#ifndef PSG_TESTS_HOST_FAKES_FAKE_UART_H
#define PSG_TESTS_HOST_FAKES_FAKE_UART_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "drv/uart/uart.h"
#include "fakes/fake_timebase.h"

#define FAKE_UART_TX_QUEUE 8u
#define FAKE_UART_FRAME_BYTES 512u

typedef struct {
    uart_t base;
    uart_data_t data;
    uart_config_t config;
    const fake_timebase_t *clock;

    uint64_t tx_start_us;
    uint64_t tx_duration_us;

    uint8_t frames[FAKE_UART_TX_QUEUE][FAKE_UART_FRAME_BYTES];
    size_t lengths[FAKE_UART_TX_QUEUE];
    uint8_t frame_head;
    uint8_t frame_count;
    uint32_t frames_written;
} fake_uart_t;

void fake_uart_init(fake_uart_t *fake, const fake_timebase_t *clock);
void fake_uart_push_byte(fake_uart_t *fake, uint8_t value, uint32_t timestamp_us);
void fake_uart_push_bytes(fake_uart_t *fake, const uint8_t *bytes, size_t count, uint32_t timestamp_us);

/* Oldest frame handed to the device, removed from the queue. */
bool fake_uart_take_tx(fake_uart_t *fake, uint8_t *out, size_t capacity, size_t *length);
bool fake_uart_tx_empty(const fake_uart_t *fake);

#endif /* PSG_TESTS_HOST_FAKES_FAKE_UART_H */
