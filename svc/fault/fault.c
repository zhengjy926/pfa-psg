#include "svc/fault/fault.h"

void psg_fault_init(psg_fault_t *fault)
{
    fault->word = 0u;
}

uint8_t psg_fault_word(const psg_fault_t *fault)
{
    return fault->word;
}

void psg_fault_set(psg_fault_t *fault, uint8_t bits)
{
    fault->word = (uint8_t)(fault->word | bits);
}

void psg_fault_clear(psg_fault_t *fault, uint8_t bits)
{
    fault->word = (uint8_t)(fault->word & (uint8_t)(~bits));
}

bool psg_fault_has_new_bits(uint8_t previous, uint8_t current)
{
    return (current & (uint8_t)(~previous)) != 0u;
}
