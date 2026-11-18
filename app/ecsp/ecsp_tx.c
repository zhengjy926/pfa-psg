#include "app/ecsp/ecsp_tx.h"

#include <string.h>

#include "app/ecsp/ecsp_codes.h"
#include "app/ecsp/ecsp_frame.h"

static uint8_t ring_next(uint8_t index, uint8_t depth)
{
    return (uint8_t)((index + 1u) % depth);
}

static bool item_submit(ecsp_tx_item_t *items, uint8_t *head, uint8_t *count, uint8_t depth,
                        uint8_t command, const uint8_t *payload, uint16_t payload_length,
                        uint32_t *overflow)
{
    uint8_t slot;

    if (payload_length > ECSP_TX_SMALL_PAYLOAD_BYTES) {
        return false;
    }
    if ((payload_length != 0u) && (payload == NULL)) {
        return false;
    }

    if (*count == depth) {
        /* Replace the oldest entry; the protocol keeps 4 slots so this only
         * happens under a deliberate flood. */
        *head = ring_next(*head, depth);
        (*count)--;
        (*overflow)++;
    }
    slot = (uint8_t)((*head + *count) % depth);
    items[slot].command = command;
    items[slot].length = (uint8_t)payload_length;
    if (payload_length != 0u) {
        (void)memcpy(items[slot].payload, payload, (size_t)payload_length);
    }
    (*count)++;
    return true;
}

void ecsp_tx_init(ecsp_tx_t *tx, const uart_t *uart, uint8_t product, uint8_t board, uint64_t now_us)
{
    (void)memset(tx, 0, sizeof(*tx));
    tx->uart = uart;
    tx->product = product;
    tx->board = board;
    tx->boot_pending = true;
    tx->boot_due_us = now_us;
    tx->frame_kind = ECSP_TX_KIND_NONE;
}

void ecsp_tx_set_connected(ecsp_tx_t *tx, bool connected, uint64_t now_us)
{
    tx->connected = connected;
    if (connected) {
        tx->boot_pending = false;
        if (!tx->heartbeat_armed) {
            tx->heartbeat_armed = true;
            tx->heartbeat_due_us = now_us + PSG_HEARTBEAT_INTERVAL_US;
        }
    } else {
        tx->heartbeat_armed = false;
    }
}

bool ecsp_tx_submit_reply(ecsp_tx_t *tx, uint8_t command, const uint8_t *payload, uint16_t payload_length)
{
    return item_submit(tx->replies, &tx->reply_head, &tx->reply_count, PSG_ECSP_REPLY_QUEUE_DEPTH,
                       command, payload, payload_length, &tx->overflow_count);
}

bool ecsp_tx_submit_large_reply(ecsp_tx_t *tx, const uint8_t *frame, uint16_t frame_length)
{
    if ((frame == NULL) || (frame_length > PSG_ECSP_TX_FRAME_BYTES) ||
        (frame_length < PSG_ECSP_MIN_FRAME_BYTES)) {
        return false;
    }
    if (tx->big_reply_pending) {
        tx->overflow_count++;
    }
    (void)memcpy(tx->big_reply, frame, (size_t)frame_length);
    tx->big_reply_length = frame_length;
    tx->big_reply_pending = true;
    return true;
}

bool ecsp_tx_submit_event(ecsp_tx_t *tx, uint8_t command, const uint8_t *payload, uint16_t payload_length)
{
    return item_submit(tx->events, &tx->event_head, &tx->event_count, PSG_ECSP_EVENT_QUEUE_DEPTH,
                       command, payload, payload_length, &tx->overflow_count);
}

void ecsp_tx_submit_fault(ecsp_tx_t *tx, uint8_t fault_word)
{
    if (fault_word == 0u) {
        return;
    }
    tx->fault_word = fault_word;
    tx->fault_pending = true;
}

void ecsp_tx_request_reset_after_reply(ecsp_tx_t *tx)
{
    tx->reset_after_reply = true;
}

bool ecsp_tx_take_reset(ecsp_tx_t *tx)
{
    bool ready = tx->reset_ready;

    tx->reset_ready = false;
    return ready;
}

void ecsp_tx_flush(ecsp_tx_t *tx)
{
    tx->reply_head = 0u;
    tx->reply_count = 0u;
    tx->event_head = 0u;
    tx->event_count = 0u;
    tx->big_reply_pending = false;
    tx->big_reply_length = 0u;
}

bool ecsp_tx_idle(const ecsp_tx_t *tx)
{
    return (!tx->in_flight) && (tx->reply_count == 0u) && (tx->event_count == 0u) &&
           (!tx->big_reply_pending) && (!tx->fault_pending);
}

uint32_t ecsp_tx_overflow_count(const ecsp_tx_t *tx)
{
    return tx->overflow_count;
}

static bool tx_start(ecsp_tx_t *tx, ecsp_tx_kind_t kind, uint16_t length)
{
    if (!uart_write(tx->uart, tx->frame, (size_t)length)) {
        return false;
    }
    tx->frame_length = length;
    tx->frame_kind = kind;
    tx->in_flight = true;
    return true;
}

static bool tx_send_small(ecsp_tx_t *tx, ecsp_tx_kind_t kind, uint8_t command, const uint8_t *payload,
                          uint8_t payload_length)
{
    size_t length = ecsp_frame_encode(tx->frame, sizeof(tx->frame), tx->product, tx->board, command,
                                      payload, payload_length);

    if (length == 0u) {
        return false;
    }
    return tx_start(tx, kind, (uint16_t)length);
}

static void tx_on_completion(ecsp_tx_t *tx, uint64_t completion_us, ecsp_tx_kind_t kind)
{
    tx->in_flight = false;
    tx->frame_kind = ECSP_TX_KIND_NONE;

    if (kind == ECSP_TX_KIND_REPLY) {
        if (tx->reset_after_reply) {
            tx->reset_after_reply = false;
            tx->reset_ready = true;
        }
    }
    if ((kind == ECSP_TX_KIND_HANDSHAKE_REPORT) && (!tx->connected)) {
        tx->boot_pending = true;
        tx->boot_due_us = completion_us + PSG_HANDSHAKE_REPORT_INTERVAL_US;
    }
    if (tx->connected) {
        tx->heartbeat_armed = true;
        tx->heartbeat_due_us = completion_us + PSG_HEARTBEAT_INTERVAL_US;
    }
}

static bool tx_try_heartbeat(ecsp_tx_t *tx, uint64_t now_us)
{
    uint64_t duration;
    uint64_t start_at;

    if ((!tx->connected) || (!tx->heartbeat_armed)) {
        return false;
    }
    duration = psg_uart_frame_duration_us(PSG_ECSP_MIN_FRAME_BYTES);
    start_at = (tx->heartbeat_due_us > duration) ? (tx->heartbeat_due_us - duration) : 0u;
    if (!psg_time_reached(now_us, start_at)) {
        return false;
    }
    return tx_send_small(tx, ECSP_TX_KIND_HEARTBEAT, ECSP_CMD_HEARTBEAT, NULL, 0u);
}

static bool tx_try_boot_report(ecsp_tx_t *tx, uint64_t now_us)
{
    uint64_t duration;
    uint64_t start_at;

    if ((tx->connected) || (!tx->boot_pending)) {
        return false;
    }
    duration = psg_uart_frame_duration_us(PSG_ECSP_MIN_FRAME_BYTES);
    start_at = (tx->boot_due_us > duration) ? (tx->boot_due_us - duration) : 0u;
    if (!psg_time_reached(now_us, start_at)) {
        return false;
    }
    return tx_send_small(tx, ECSP_TX_KIND_HANDSHAKE_REPORT, ECSP_CMD_HANDSHAKE, NULL, 0u);
}

void ecsp_tx_pump(ecsp_tx_t *tx, uint64_t now_us)
{
    uint64_t completion_us;

    /* Completion is observed before a new frame is chosen so the heartbeat
     * baseline is the actually completed frame (T-010). */
    if (uart_poll_tx_complete(tx->uart, &completion_us)) {
        tx_on_completion(tx, completion_us, tx->frame_kind);
    }

    if (tx->in_flight) {
        return;
    }

    if (tx->big_reply_pending) {
        (void)memcpy(tx->frame, tx->big_reply, (size_t)tx->big_reply_length);
        if (tx_start(tx, ECSP_TX_KIND_REPLY, tx->big_reply_length)) {
            tx->big_reply_pending = false;
            tx->big_reply_length = 0u;
        }
        return;
    }

    if (tx->reply_count != 0u) {
        const ecsp_tx_item_t *item = &tx->replies[tx->reply_head];
        if (tx_send_small(tx, ECSP_TX_KIND_REPLY, item->command, item->payload, item->length)) {
            tx->reply_head = ring_next(tx->reply_head, PSG_ECSP_REPLY_QUEUE_DEPTH);
            tx->reply_count--;
        }
        return;
    }

    if (tx->fault_pending) {
        if (tx_send_small(tx, ECSP_TX_KIND_FAULT, ECSP_CMD_FAULT_REPORT, &tx->fault_word, 1u)) {
            tx->fault_pending = false;
        }
        return;
    }

    if (tx_try_heartbeat(tx, now_us)) {
        return;
    }

    if (tx->event_count != 0u) {
        const ecsp_tx_item_t *item = &tx->events[tx->event_head];
        if (tx_send_small(tx, ECSP_TX_KIND_EVENT, item->command, item->payload, item->length)) {
            tx->event_head = ring_next(tx->event_head, PSG_ECSP_EVENT_QUEUE_DEPTH);
            tx->event_count--;
        }
        return;
    }

    (void)tx_try_boot_report(tx, now_us);
}
