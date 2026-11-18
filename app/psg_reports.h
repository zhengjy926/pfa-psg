/*
 * Shared 0x31 / 0x3C output status payload (PRM-209).
 */
#ifndef PSG_APP_PSG_REPORTS_H
#define PSG_APP_PSG_REPORTS_H

#include <stdint.h>

#include "run/pulse_engine.h"

#define PSG_STATUS_BLOCK_BYTES 13u

/* Response code is not part of the block; callers add it for 0x31. */
void psg_reports_pack_status(const psg_output_status_t *status, uint8_t out[PSG_STATUS_BLOCK_BYTES]);

#endif /* PSG_APP_PSG_REPORTS_H */
