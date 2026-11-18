/*
 * Little-endian payload codecs shared by the ECSP framing layer and the run
 * configuration validators (PRM-014).  Living in shared/ keeps run/ free of
 * an app/ include.
 */
#ifndef PSG_SHARED_PSG_BYTES_H
#define PSG_SHARED_PSG_BYTES_H

#include <stdint.h>

void ecsp_write_u16(uint8_t *out, uint16_t value);
void ecsp_write_u32(uint8_t *out, uint32_t value);
void ecsp_write_f32(uint8_t *out, float value);
uint16_t ecsp_read_u16(const uint8_t *in);
uint32_t ecsp_read_u32(const uint8_t *in);
float ecsp_read_f32(const uint8_t *in);

#endif /* PSG_SHARED_PSG_BYTES_H */
