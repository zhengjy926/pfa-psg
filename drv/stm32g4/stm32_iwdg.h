#ifndef PSG_DRV_STM32G4_STM32_IWDG_H
#define PSG_DRV_STM32G4_STM32_IWDG_H

#include "drv/iwdg/iwdg.h"

typedef struct {
    iwdg_t base;
} stm32_iwdg_t;

void stm32_iwdg_init(stm32_iwdg_t *impl);

#endif /* PSG_DRV_STM32G4_STM32_IWDG_H */
