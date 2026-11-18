#ifndef PSG_TESTS_HOST_FAKES_FAKE_LED_H
#define PSG_TESTS_HOST_FAKES_FAKE_LED_H

#include <stdbool.h>
#include <stdint.h>

#include "drv/led/led.h"

typedef struct {
    led_t base;
    bool on;
    uint32_t transitions;
} fake_led_t;

void fake_led_init(fake_led_t *fake);

#endif /* PSG_TESTS_HOST_FAKES_FAKE_LED_H */
