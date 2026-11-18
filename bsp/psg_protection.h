/*
 * Output-stop single-owner matrix (architecture section 5).
 *
 * Firmware never writes an output register from an ISR: over-current stops the
 * outputs in the HRTIM fault state and the port input stops them through EEV7.
 * This header only exposes the evidence reads and the latch clear pulse.
 */
#ifndef PSG_BSP_PSG_PROTECTION_H
#define PSG_BSP_PSG_PROTECTION_H

#include <stdbool.h>
#include <stdint.h>

/* Bit assignments for the protection evidence word. */
#define PSG_PROTECTION_OVERCURRENT_POS 0x01u
#define PSG_PROTECTION_OVERCURRENT_NEG 0x02u
#define PSG_PROTECTION_LATCH_POS 0x04u
#define PSG_PROTECTION_LATCH_NEG 0x08u

/* Read the live protection evidence; bits are the PSG_PROTECTION_* mask. */
uint8_t psg_protection_evidence(void);

/* Assert the over-current latch reset line (PG9); the caller waits PRM-512
 * before reading the latches again. */
void psg_protection_latch_reset(void);

#endif /* PSG_BSP_PSG_PROTECTION_H */
