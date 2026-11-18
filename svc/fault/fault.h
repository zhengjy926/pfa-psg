/*
 * PSG fault word (PRM-208, board spec section 4).
 *
 * Only bit0 is assigned; 0x12 reads the whole word without clearing it
 * (C-4-05).  0x11 reports a fault word only when a bit changes from 0 to 1.
 */
#ifndef PSG_SVC_FAULT_FAULT_H
#define PSG_SVC_FAULT_FAULT_H

#include <stdbool.h>
#include <stdint.h>

#define PSG_FAULT_OVERCURRENT_LATCH 0x01u

typedef struct {
    uint8_t word;
} psg_fault_t;

void psg_fault_init(psg_fault_t *fault);
uint8_t psg_fault_word(const psg_fault_t *fault);
void psg_fault_set(psg_fault_t *fault, uint8_t bits);
void psg_fault_clear(psg_fault_t *fault, uint8_t bits);

/* True when current has at least one bit that previous did not. */
bool psg_fault_has_new_bits(uint8_t previous, uint8_t current);

#endif /* PSG_SVC_FAULT_FAULT_H */
