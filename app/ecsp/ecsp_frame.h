/*
 * ECSP frame fields, CRC and payload encoding (I-002..I-005).
 */
#ifndef PSG_APP_ECSP_ECSP_FRAME_H
#define PSG_APP_ECSP_ECSP_FRAME_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "shared/psg_bytes.h"
#include "shared/psg_config.h"

typedef struct {
    uint8_t product;
    uint8_t command;
    uint8_t board;
    uint16_t payload_length;
    uint8_t payload[PSG_ECSP_MAX_PAYLOAD_BYTES];
} ecsp_frame_t;

/* CRC16-MODBUS, PRM-015..PRM-021.  Self-check: "123456789" -> 0x4B37. */
uint16_t ecsp_crc16(const uint8_t *data, size_t length);

/* Header helpers. */
uint16_t ecsp_frame_length_field(const uint8_t *header);

/* Build one frame.  Returns the encoded length, or 0 if the payload or the
 * output capacity is out of range. */
size_t ecsp_frame_encode(uint8_t *out, size_t capacity, uint8_t product, uint8_t board, uint8_t command,
                         const uint8_t *payload, uint16_t payload_length);

/* Two-step form for large payloads built in place: begin() writes the header
 * and returns the total frame length, the caller fills the payload at offset
 * 6, and finish() writes the CRC and tail. */
size_t ecsp_frame_begin(uint8_t *out, size_t capacity, uint8_t product, uint8_t board, uint8_t command,
                        uint16_t payload_length);
void ecsp_frame_finish(uint8_t *out, size_t frame_length);

/* Validate and copy one complete frame: head, length, tail, CRC and bounds. */
bool ecsp_frame_parse(const uint8_t *bytes, size_t length, ecsp_frame_t *frame);

static inline bool ecsp_payload_length_valid(uint16_t length, uint16_t expected)
{
    return length == expected;
}

#endif /* PSG_APP_ECSP_ECSP_FRAME_H */
