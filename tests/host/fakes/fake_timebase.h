#ifndef PSG_TESTS_HOST_FAKES_FAKE_TIMEBASE_H
#define PSG_TESTS_HOST_FAKES_FAKE_TIMEBASE_H

#include <stdint.h>

#include "drv/timebase/timebase.h"

typedef struct {
    timebase_t base;
    uint64_t now_us;
} fake_timebase_t;

void fake_timebase_init(fake_timebase_t *fake, uint64_t now_us);
void fake_timebase_set(fake_timebase_t *fake, uint64_t now_us);
void fake_timebase_advance(fake_timebase_t *fake, uint64_t delta_us);

#endif /* PSG_TESTS_HOST_FAKES_FAKE_TIMEBASE_H */
