#include "shared/psg_bytes.h"

#include <string.h>

_Static_assert(sizeof(float) == 4u, "ECSP float32 encoding needs 32-bit float");

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
