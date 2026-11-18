#ifndef PSG_DRV_STM32G4_STM32_WATCHDOG_H
#define PSG_DRV_STM32G4_STM32_WATCHDOG_H

#include "drv/iwdg/iwdg.h"
#include "drv/watchdog/watchdog.h"

typedef struct {
    watchdog_t base;
    const iwdg_t *iwdg;
} stm32_watchdog_t;

void stm32_watchdog_init(stm32_watchdog_t *impl, const iwdg_t *iwdg);

#endif /* PSG_DRV_STM32G4_STM32_WATCHDOG_H */
