#include "app/ecsp/ecsp_frame.h"

#include <string.h>

#define ECSP_LENGTH_OFFSET 1u
#define ECSP_PRODUCT_OFFSET 3u
#define ECSP_COMMAND_OFFSET 4u
#define ECSP_BOARD_OFFSET 5u
#define ECSP_PAYLOAD_OFFSET 6u
#define ECSP_FIXED_OVERHEAD (PSG_ECSP_HEADER_BYTES + PSG_ECSP_CRC_BYTES + 1u)

_Static_assert(sizeof(float) == 4u, "ECSP float32 encoding needs 32-bit float");

uint16_t ecsp_crc16(const uint8_t *data, size_t length)
{
    uint16_t crc = 0xFFFFu;
    size_t index;

    for (index = 0u; index < length; index++) {
        uint8_t bit;
        crc ^= (uint16_t)data[index];
        for (bit = 0u; bit < 8u; bit++) {
            if ((crc & 1u) != 0u) {
                crc = (uint16_t)((crc >> 1u) ^ 0xA001u);
            } else {
                crc = (uint16_t)(crc >> 1u);
            }
        }
    }
    return crc;
}

uint16_t ecsp_frame_length_field(const uint8_t *header)
{
    return (uint16_t)((uint16_t)header[ECSP_LENGTH_OFFSET] |
                      (uint16_t)((uint16_t)header[ECSP_LENGTH_OFFSET + 1u] << 8u));
}

size_t ecsp_frame_encode(uint8_t *out, size_t capacity, uint8_t product, uint8_t board, uint8_t command,
                         const uint8_t *payload, uint16_t payload_length)
{
    size_t frame_length;

    if ((payload_length != 0u) && (payload == NULL)) {
        return 0u;
    }
    frame_length = ecsp_frame_begin(out, capacity, product, board, command, payload_length);
    if (frame_length == 0u) {
        return 0u;
    }
    if (payload_length != 0u) {
        (void)memcpy(&out[ECSP_PAYLOAD_OFFSET], payload, (size_t)payload_length);
    }
    ecsp_frame_finish(out, frame_length);
    return frame_length;
}

size_t ecsp_frame_begin(uint8_t *out, size_t capacity, uint8_t product, uint8_t board, uint8_t command,
                        uint16_t payload_length)
{
    size_t frame_length;

    if ((out == NULL) || (payload_length > PSG_ECSP_MAX_PAYLOAD_BYTES)) {
        return 0u;
    }
    frame_length = (size_t)payload_length + (size_t)ECSP_FIXED_OVERHEAD;
    if ((frame_length < PSG_ECSP_MIN_FRAME_BYTES) || (frame_length > PSG_ECSP_MAX_FRAME_BYTES) ||
        (frame_length > capacity)) {
        return 0u;
    }
    out[0] = PSG_ECSP_HEAD;
    out[ECSP_LENGTH_OFFSET] = (uint8_t)(frame_length & 0xFFu);
    out[ECSP_LENGTH_OFFSET + 1u] = (uint8_t)((frame_length >> 8u) & 0xFFu);
    out[ECSP_PRODUCT_OFFSET] = product;
    out[ECSP_COMMAND_OFFSET] = command;
    out[ECSP_BOARD_OFFSET] = board;
    return frame_length;
}

void ecsp_frame_finish(uint8_t *out, size_t frame_length)
{
    uint16_t crc = ecsp_crc16(&out[ECSP_LENGTH_OFFSET], frame_length - 4u);

    out[frame_length - 3u] = (uint8_t)(crc & 0xFFu);
    out[frame_length - 2u] = (uint8_t)((crc >> 8u) & 0xFFu);
    out[frame_length - 1u] = PSG_ECSP_TAIL;
}

bool ecsp_frame_parse(const uint8_t *bytes, size_t length, ecsp_frame_t *frame)
{
    uint16_t declared_length;
    uint16_t crc_claimed;
    uint16_t crc_computed;
    uint16_t payload_length;

    if ((bytes == NULL) || (frame == NULL) || (length < PSG_ECSP_MIN_FRAME_BYTES) ||
        (length > PSG_ECSP_MAX_FRAME_BYTES)) {
        return false;
    }
    if ((bytes[0] != PSG_ECSP_HEAD) || (bytes[length - 1u] != PSG_ECSP_TAIL)) {
        return false;
    }
    declared_length = ecsp_frame_length_field(bytes);
    if ((size_t)declared_length != length) {
        return false;
    }

    payload_length = (uint16_t)(length - (size_t)ECSP_FIXED_OVERHEAD);
    if (payload_length > PSG_ECSP_MAX_PAYLOAD_BYTES) {
        return false;
    }

    crc_claimed = (uint16_t)((uint16_t)bytes[length - 3u] | (uint16_t)((uint16_t)bytes[length - 2u] << 8u));
    crc_computed = ecsp_crc16(&bytes[ECSP_LENGTH_OFFSET], length - 4u);
    if (crc_claimed != crc_computed) {
        return false;
    }

    frame->product = bytes[ECSP_PRODUCT_OFFSET];
    frame->command = bytes[ECSP_COMMAND_OFFSET];
    frame->board = bytes[ECSP_BOARD_OFFSET];
    frame->payload_length = payload_length;
    if (payload_length != 0u) {
        (void)memcpy(frame->payload, &bytes[ECSP_PAYLOAD_OFFSET], (size_t)payload_length);
    }
    return true;
}

void ecsp_write_u16(uint8_t *out, uint16_t value)
{
    out[0] = (uint8_t)(value & 0xFFu);
    out[1] = (uint8_t)((value >> 8u) & 0xFFu);
}

void ecsp_write_u32(uint8_t *out, uint32_t value)
{
    out[0] = (uint8_t)(value & 0xFFu);
    out[1] = (uint8_t)((value >> 8u) & 0xFFu);
    out[2] = (uint8_t)((value >> 16u) & 0xFFu);
    out[3] = (uint8_t)((value >> 24u) & 0xFFu);
}

void ecsp_write_f32(uint8_t *out, float value)
{
    uint32_t raw;

    (void)memcpy(&raw, &value, sizeof(raw));
    ecsp_write_u32(out, raw);
}

uint16_t ecsp_read_u16(const uint8_t *in)
{
    return (uint16_t)((uint16_t)in[0] | (uint16_t)((uint16_t)in[1] << 8u));
}

uint32_t ecsp_read_u32(const uint8_t *in)
{
    return (uint32_t)in[0] | ((uint32_t)in[1] << 8u) | ((uint32_t)in[2] << 16u) | ((uint32_t)in[3] << 24u);
}

float ecsp_read_f32(const uint8_t *in)
{
    uint32_t raw = ecsp_read_u32(in);
    float value;

    (void)memcpy(&value, &raw, sizeof(value));
    return value;
}
