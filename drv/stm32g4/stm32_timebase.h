/*
 * STM32G4 monotonic time base: TIM2 at 1 MHz (PRM-707).
 *
 * TIM2 is 32-bit and wraps every ~71 minutes.  The task-side reader extends it
 * to 64 bits; the UART interrupt reads the raw counter so receive stamps share
 * the same time origin without touching the extension.
 */
#ifndef PSG_DRV_STM32G4_STM32_TIMEBASE_H
#define PSG_DRV_STM32G4_STM32_TIMEBASE_H

#include <stdint.h>

#include "drv/timebase/timebase.h"

typedef struct {
    timebase_t base;
    uint64_t accumulated_us;
    uint32_t last_counter;
} stm32_timebase_t;

void stm32_timebase_init(stm32_timebase_t *impl);

/* Raw 1 MHz counter, safe to call from an interrupt. */
uint32_t stm32_timebase_counter(void);

#endif /* PSG_DRV_STM32G4_STM32_TIMEBASE_H */
