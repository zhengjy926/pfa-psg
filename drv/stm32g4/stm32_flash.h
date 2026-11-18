#ifndef PSG_DRV_STM32G4_STM32_FLASH_H
#define PSG_DRV_STM32G4_STM32_FLASH_H

#include "drv/flash/flash.h"

typedef struct {
    flash_t base;
} stm32_flash_t;

void stm32_flash_init(stm32_flash_t *impl);

#endif /* PSG_DRV_STM32G4_STM32_FLASH_H */
