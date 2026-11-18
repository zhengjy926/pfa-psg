/*
 * Shared UART receive-ring and transmit-completion state helpers.
 *
 * The target DMA/IDLE interrupt and the host test double drive the same
 * ring, so overflow, gap reporting and completion bookkeeping have exactly
 * one implementation (HW-03, T-010).
 */
#ifndef PSG_DRV_UART_RING_H
#define PSG_DRV_UART_RING_H

#include "drv/uart/uart.h"

/* Push one received byte.  On a full ring the oldest byte is dropped and the
 * gap counter is incremented; the new byte is always stored. */
void uart_ring_push(uart_data_t *data, uint8_t value, uint32_t timestamp_us);

/* Pop up to capacity bytes, oldest first. */
size_t uart_ring_pop(uart_data_t *data, uint8_t *values, uint32_t *timestamps, size_t capacity);

/* Read and clear the accumulated dropped-byte count. */
uint32_t uart_ring_take_gaps(uart_data_t *data);

/* Transmit bookkeeping.  begin() records the in-flight frame; the driver
 * calls mark_complete() from the USART TC interrupt or, on the host, from a
 * poll of the scripted clock. */
void uart_tx_begin(uart_data_t *data, const uint8_t *frame, uint32_t length);
void uart_tx_mark_complete(uart_data_t *data, uint64_t completion_us);
bool uart_tx_poll_complete(uart_data_t *data, uint64_t *completion_us);
bool uart_tx_is_busy(const uart_data_t *data);

#endif /* PSG_DRV_UART_RING_H */
