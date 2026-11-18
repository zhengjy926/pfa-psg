#ifndef PSG_TESTS_HOST_FAKES_FAKE_FLASH_H
#define PSG_TESTS_HOST_FAKES_FAKE_FLASH_H

#include <stdbool.h>
#include <stdint.h>

#include "drv/flash/flash.h"

#define FAKE_FLASH_BYTES 4096u

typedef struct {
    flash_t base;
    uint8_t memory[FAKE_FLASH_BYTES];
    bool busy;
    flash_status_t result;
} fake_flash_t;

void fake_flash_init(fake_flash_t *fake);

#endif /* PSG_TESTS_HOST_FAKES_FAKE_FLASH_H */
