/*
 * PSG ECSP service: receive, session, dedup, adjudication and transmit
 * (SRS I-002..I-012, architecture section 2).
 *
 * The service is a single-owner state machine.  The UART task calls
 * psg_ecsp_feed(); the protocol task calls psg_ecsp_step().  All command
 * adjudication happens in one ordered pipeline here:
 *   frame/address -> command support -> handshake -> length/parameters ->
 *   dedup replay -> state matrix -> execution -> reply.
 */
#ifndef PSG_APP_PSG_ECSP_H
#define PSG_APP_PSG_ECSP_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "app/ecsp/ecsp_frame.h"
#include "app/ecsp/ecsp_rx.h"
#include "app/ecsp/ecsp_session.h"
#include "app/ecsp/ecsp_tx.h"
#include "bsp/psg_board.h"
#include "drv/uart/uart.h"
#include "run/pulse_engine.h"
#include "svc/dedup/dedup.h"
#include "svc/fault/fault.h"
#include "svc/identity/identity.h"

typedef struct {
    const uart_t *uart;
    psg_board_t board;
    psg_run_t *run;
    identity_t *identity;
    psg_fault_t *fault;
    uint8_t product_address;
    uint8_t board_address;
} psg_ecsp_config_t;

typedef struct {
    psg_ecsp_config_t config;
    ecsp_rx_t rx;
    ecsp_tx_t tx;
    ecsp_session_t session;
    dedup_t dedup;
    ecsp_frame_t frame;
    psg_sequence_t sequence_candidate;
    uint8_t large_response[PSG_ECSP_TX_FRAME_BYTES];
    uint8_t last_fault_word;
    bool reset_pending;
    bool clear_pending;
    uint32_t overflow_seen;
} psg_ecsp_t;

void psg_ecsp_init(psg_ecsp_t *svc, const psg_ecsp_config_t *config, uint64_t now_us);

/* UART task hand-off. */
void psg_ecsp_feed(psg_ecsp_t *svc, const uint8_t *values, const uint32_t *timestamps, size_t count,
                   uint32_t gaps);

/* Protocol task step: receive, dispatch, upload gating, transmit. */
void psg_ecsp_step(psg_ecsp_t *svc, uint64_t now_us);

#endif /* PSG_APP_PSG_ECSP_H */
