#include "app/ecsp/ecsp_rx.h"

#include <string.h>

#include "drv/timebase/timebase.h"

static void rx_compact(ecsp_rx_t *rx)
{
    if (rx->position >= rx->length) {
        rx->length = 0u;
        rx->position = 0u;
        return;
    }
    if (rx->position == 0u) {
        return;
    }
    rx->length = (uint16_t)(rx->length - rx->position);
    (void)memmove(&rx->values[0], &rx->values[rx->position], (size_t)rx->length);
    (void)memmove(&rx->timestamps[0], &rx->timestamps[rx->position],
                  (size_t)rx->length * sizeof(rx->timestamps[0]));
    rx->position = 0u;
}

static void rx_drop_oldest(ecsp_rx_t *rx)
{
    (void)memmove(&rx->values[0], &rx->values[1], (size_t)(rx->length - 1u));
    (void)memmove(&rx->timestamps[0], &rx->timestamps[1],
                  (size_t)(rx->length - 1u) * sizeof(rx->timestamps[0]));
    rx->length--;
    if (rx->position > 0u) {
        rx->position--;
    }
    rx->overflow_count++;
    rx->resync = true;
}

void ecsp_rx_init(ecsp_rx_t *rx)
{
    rx->length = 0u;
    rx->position = 0u;
    rx->overflow_count = 0u;
    rx->resync = false;
    rx->initialized = true;
}

size_t ecsp_rx_feed(ecsp_rx_t *rx, const uart_rx_byte_t *bytes, size_t count)
{
    size_t accepted = 0u;
    size_t index;

    if ((rx == NULL) || (bytes == NULL) || (count == 0u)) {
        return 0u;
    }

    rx_compact(rx);

    for (index = 0u; index < count; index++) {
        if (rx->length >= (uint16_t)PSG_ECSP_RX_WINDOW_BYTES) {
            rx_drop_oldest(rx);
        }
        rx->values[rx->length] = bytes[index].value;
        rx->timestamps[rx->length] = bytes[index].timestamp_us;
        rx->length++;
        accepted++;
    }
    return accepted;
}

void ecsp_rx_note_gap(ecsp_rx_t *rx)
{
    if (rx == NULL) {
        return;
    }
    rx->overflow_count++;
    rx->resync = true;
}

uint32_t ecsp_rx_overflow_count(const ecsp_rx_t *rx)
{
    return (rx == NULL) ? 0u : rx->overflow_count;
}

bool ecsp_rx_poll(ecsp_rx_t *rx, uint32_t now_us, ecsp_frame_t *out)
{
    if ((rx == NULL) || (out == NULL) || (rx->length == 0u)) {
        return false;
    }

    if (rx->resync) {
        rx->position = 0u;
        rx->resync = false;
    }

    for (;;) {
        uint16_t head = rx->position;
        uint16_t available;
        uint16_t declared;
        uint32_t deadline;

        /* Find the next candidate frame head. */
        while ((head < rx->length) && (rx->values[head] != PSG_ECSP_HEAD)) {
            head++;
        }
        if (head >= rx->length) {
            rx->length = 0u;
            rx->position = 0u;
            return false;
        }
        rx->position = head;
        available = (uint16_t)(rx->length - head);
        deadline = rx->timestamps[head] + (uint32_t)PSG_HALF_PACKET_TIMEOUT_US;

        if (available < 3u) {
            if (psg_time32_reached(now_us, deadline)) {
                rx->position = (uint16_t)(head + 1u);
                continue;
            }
            return false;
        }

        declared = ecsp_frame_length_field(&rx->values[head]);
        if ((declared < PSG_ECSP_MIN_FRAME_BYTES) || (declared > PSG_ECSP_MAX_FRAME_BYTES)) {
            /* Out-of-range length is rejected as soon as the field is known;
             * the bytes from the next one are rescanned immediately. */
            rx->position = (uint16_t)(head + 1u);
            continue;
        }

        if (available < declared) {
            if (psg_time32_reached(now_us, deadline)) {
                rx->position = (uint16_t)(head + 1u);
                continue;
            }
            return false;
        }

        /* The frame is fully buffered; it is only valid if its last byte was
         * received at or before the candidate deadline. */
        if (psg_time32_after(rx->timestamps[head + declared - 1u], deadline)) {
            rx->position = (uint16_t)(head + 1u);
            continue;
        }
        if (rx->values[head + declared - 1u] != PSG_ECSP_TAIL) {
            rx->position = (uint16_t)(head + 1u);
            continue;
        }
        if (!ecsp_frame_parse(&rx->values[head], (size_t)declared, out)) {
            rx->position = (uint16_t)(head + 1u);
            continue;
        }

        rx->position = (uint16_t)(head + declared);
        rx_compact(rx);
        return true;
    }
}
