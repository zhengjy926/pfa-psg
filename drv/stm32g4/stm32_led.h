#ifndef PSG_DRV_STM32G4_STM32_LED_H
#define PSG_DRV_STM32G4_STM32_LED_H

#include "drv/led/led.h"

typedef struct {
    led_t base;
} stm32_led_t;

void stm32_led_init(stm32_led_t *impl);

#endif /* PSG_DRV_STM32G4_STM32_LED_H */
