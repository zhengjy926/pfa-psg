/*
 * Transaction dedup FIFO for non-idempotent actions (PRM-027..PRM-029,
 * I-007).
 *
 * A hit replays the stored response and never runs the business action again.
 * Entries leave the FIFO when it is full (oldest replaced), on handshake
 * acceptance, and on reset, which is the "most recent N transactions" window
 * required for sequence wrap.
 */
#ifndef PSG_SVC_DEDUP_DEDUP_H
#define PSG_SVC_DEDUP_DEDUP_H

#include <stdbool.h>
#include <stdint.h>

#include "shared/psg_config.h"

typedef struct {
    uint8_t sequence;
    uint8_t command;
    uint8_t code;
    uint8_t payload_length;
    uint8_t payload[PSG_DEDUP_PAYLOAD_BYTES];
    bool valid;
} dedup_entry_t;

typedef struct {
    dedup_entry_t entries[PSG_DEDUP_DEPTH];
    uint8_t head;
    uint8_t count;
    uint32_t overflow_count;
} dedup_t;

void dedup_init(dedup_t *dedup);
void dedup_clear(dedup_t *dedup);

bool dedup_lookup(const dedup_t *dedup, uint8_t command, uint8_t sequence, dedup_entry_t *entry);

/* Store the response of a request that passed the public and payload checks. */
void dedup_store(dedup_t *dedup, uint8_t command, uint8_t sequence, uint8_t code,
                 const uint8_t *payload, uint8_t payload_length);

uint32_t dedup_overflow_count(const dedup_t *dedup);

#endif /* PSG_SVC_DEDUP_DEDUP_H */
