#include "svc/dedup/dedup.h"

#include <string.h>

static uint8_t ring_next(uint8_t index)
{
    return (uint8_t)((index + 1u) % PSG_DEDUP_DEPTH);
}

void dedup_init(dedup_t *dedup)
{
    (void)memset(dedup, 0, sizeof(*dedup));
}

void dedup_clear(dedup_t *dedup)
{
    (void)memset(dedup->entries, 0, sizeof(dedup->entries));
    dedup->head = 0u;
    dedup->count = 0u;
}

bool dedup_lookup(const dedup_t *dedup, uint8_t command, uint8_t sequence, dedup_entry_t *entry)
{
    uint8_t index;
    uint8_t slot;

    for (index = 0u; index < dedup->count; index++) {
        slot = (uint8_t)((dedup->head + index) % PSG_DEDUP_DEPTH);
        if ((dedup->entries[slot].valid) && (dedup->entries[slot].command == command) &&
            (dedup->entries[slot].sequence == sequence)) {
            if (entry != NULL) {
                *entry = dedup->entries[slot];
            }
            return true;
        }
    }
    return false;
}

void dedup_store(dedup_t *dedup, uint8_t command, uint8_t sequence, uint8_t code,
                 const uint8_t *payload, uint8_t payload_length)
{
    uint8_t slot;

    if (payload_length > PSG_DEDUP_PAYLOAD_BYTES) {
        payload_length = PSG_DEDUP_PAYLOAD_BYTES;
    }
    if (dedup->count == PSG_DEDUP_DEPTH) {
        /* Replace the oldest entry and count the overflow. */
        slot = dedup->head;
        dedup->head = ring_next(dedup->head);
        dedup->overflow_count++;
    } else {
        slot = (uint8_t)((dedup->head + dedup->count) % PSG_DEDUP_DEPTH);
        dedup->count++;
    }

    dedup->entries[slot].sequence = sequence;
    dedup->entries[slot].command = command;
    dedup->entries[slot].code = code;
    dedup->entries[slot].payload_length = payload_length;
    if (payload_length != 0u) {
        (void)memcpy(dedup->entries[slot].payload, payload, (size_t)payload_length);
    }
    dedup->entries[slot].valid = true;
}

uint32_t dedup_overflow_count(const dedup_t *dedup)
{
    return dedup->overflow_count;
}
