/*
 * ECSP receive window and resynchronisation (I-004, I-005, T-011, HW-03).
 *
 * The UART task feeds timestamped bytes with ecsp_rx_feed(); the protocol task
 * parses with ecsp_rx_poll().  Only those two contexts call the module, never
 * concurrently, so no lock is needed on the single-core target.
 *
 * A candidate starts at a 0xFA byte and expires PRM-115 after that byte was
 * received.  On any failure the scan restarts at the byte after the candidate
 * head, reusing the original receive times (ECSP section 5.3).
 */
#ifndef PSG_APP_ECSP_ECSP_RX_H
#define PSG_APP_ECSP_ECSP_RX_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "app/app_config.h"
#include "app/ecsp/ecsp_frame.h"
#include "drv/uart/uart.h"

typedef struct {
    /* Parallel arrays keep the byte stream contiguous for the parser and cost
     * 5 bytes per buffered byte. */
    uint8_t values[PSG_ECSP_RX_WINDOW_BYTES];
    uint32_t timestamps[PSG_ECSP_RX_WINDOW_BYTES];
    uint16_t length;
    uint16_t position;
    uint32_t overflow_count;
    bool resync;
    bool initialized;
} ecsp_rx_t;

void ecsp_rx_init(ecsp_rx_t *rx);

/* Append received bytes.  Returns the number accepted; bytes beyond the window
 * capacity are dropped oldest-first and force a resynchronisation. */
size_t ecsp_rx_feed(ecsp_rx_t *rx, const uint8_t *values, const uint32_t *timestamps, size_t count);

/* Record a byte-stream gap reported by the UART ring (HW-03). */
void ecsp_rx_note_gap(ecsp_rx_t *rx);

/* Parse and emit at most one valid, in-time frame.  Returns true when a frame
 * was produced.  now_us is the low 32 bits of the monotonic time base. */
bool ecsp_rx_poll(ecsp_rx_t *rx, uint32_t now_us, ecsp_frame_t *out);

/* Total bytes lost to window overflow since init. */
uint32_t ecsp_rx_overflow_count(const ecsp_rx_t *rx);

#endif /* PSG_APP_ECSP_ECSP_RX_H */
