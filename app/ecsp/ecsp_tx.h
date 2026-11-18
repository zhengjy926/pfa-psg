/*
 * ECSP transmit channel (T-010, T-011, I-007).
 *
 * One frame is on the wire at a time.  Replies jump the queue of uploads, and
 * the fault session exception is kept outside the event queue so a burst of
 * 0x3C uploads can never delay it (I-028).  Heartbeat scheduling is based on
 * the completion time of the most recent frame, exactly as ECSP 6.2.2
 * requires, and the next heartbeat start is advanced by the frame duration so
 * its completion lands on the 1 s grid (PRM-111).
 */
#ifndef PSG_APP_ECSP_ECSP_TX_H
#define PSG_APP_ECSP_ECSP_TX_H

#include <stdbool.h>
#include <stdint.h>

#include "app/app_config.h"
#include "drv/timebase/timebase.h"
#include "drv/uart/uart.h"

#define ECSP_TX_SMALL_PAYLOAD_BYTES 20u

typedef enum {
    ECSP_TX_KIND_NONE = 0,
    ECSP_TX_KIND_REPLY = 1,
    ECSP_TX_KIND_EVENT = 2,
    ECSP_TX_KIND_FAULT = 3,
    ECSP_TX_KIND_HEARTBEAT = 4,
    ECSP_TX_KIND_HANDSHAKE_REPORT = 5
} ecsp_tx_kind_t;

typedef struct {
    uint8_t command;
    uint8_t length;
    bool reset_after;
    uint8_t payload[ECSP_TX_SMALL_PAYLOAD_BYTES];
} ecsp_tx_item_t;

typedef struct {
    const uart_t *uart;
    uint8_t product;
    uint8_t board;

    ecsp_tx_item_t replies[PSG_ECSP_REPLY_QUEUE_DEPTH];
    uint8_t reply_head;
    uint8_t reply_count;

    ecsp_tx_item_t events[PSG_ECSP_EVENT_QUEUE_DEPTH];
    uint8_t event_head;
    uint8_t event_count;

    uint8_t big_reply[PSG_ECSP_TX_FRAME_BYTES];
    uint16_t big_reply_length;
    bool big_reply_pending;

    uint8_t frame[PSG_ECSP_TX_FRAME_BYTES];
    uint16_t frame_length;
    ecsp_tx_kind_t frame_kind;
    bool in_flight;

    uint8_t fault_word;
    bool fault_pending;

    bool connected;
    bool boot_pending;
    uint64_t boot_due_us;

    bool heartbeat_armed;
    uint64_t heartbeat_due_us;

    bool in_flight_reset;
    bool reset_ready;

    uint32_t overflow_count;
} ecsp_tx_t;

void ecsp_tx_init(ecsp_tx_t *tx, const uart_t *uart, uint8_t product, uint8_t board, uint64_t now_us);

/* Session transition: arming the heartbeat and cancelling boot reports. */
void ecsp_tx_set_connected(ecsp_tx_t *tx, bool connected, uint64_t now_us);

bool ecsp_tx_submit_reply(ecsp_tx_t *tx, uint8_t command, const uint8_t *payload, uint16_t payload_length);

/* 0x07: this reply, and only this reply, arms the board reset on completion. */
bool ecsp_tx_submit_reset_reply(ecsp_tx_t *tx, uint8_t command, const uint8_t *payload, uint16_t payload_length);

bool ecsp_tx_submit_large_reply(ecsp_tx_t *tx, const uint8_t *frame, uint16_t frame_length);
bool ecsp_tx_submit_event(ecsp_tx_t *tx, uint8_t command, const uint8_t *payload, uint16_t payload_length);

/* 0x11 session exception: one outstanding report, newest fault word wins. */
void ecsp_tx_submit_fault(ecsp_tx_t *tx, uint8_t fault_word);

bool ecsp_tx_take_reset(ecsp_tx_t *tx);

/* Drop queued uploads and pending large replies (duplicate handshake). */
void ecsp_tx_flush(ecsp_tx_t *tx);

/* Completion bookkeeping plus at most one new transmission. */
void ecsp_tx_pump(ecsp_tx_t *tx, uint64_t now_us);

uint32_t ecsp_tx_overflow_count(const ecsp_tx_t *tx);

#endif /* PSG_APP_ECSP_ECSP_TX_H */
