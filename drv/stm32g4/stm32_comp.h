#ifndef PSG_DRV_STM32G4_STM32_COMP_H
#define PSG_DRV_STM32G4_STM32_COMP_H

#include <stdint.h>

#include "drv/comp/comp.h"

typedef struct {
    comp_t base;
    uint8_t channel; /* 0 = positive channel (PG8), 1 = negative channel (PG7) */
} stm32_comp_t;

void stm32_comp_init(stm32_comp_t *impl, uint8_t channel);

#endif /* PSG_DRV_STM32G4_STM32_COMP_H */
