#include "fakes/fake_timebase.h"

static uint64_t fake_timebase_now(const timebase_t *base)
{
    const fake_timebase_t *fake = (const fake_timebase_t *)(const void *)base;

    return fake->now_us;
}

static const timebase_ops_t g_timebase_ops = {
    .now_us = fake_timebase_now,
};

void fake_timebase_init(fake_timebase_t *fake, uint64_t now_us)
{
    fake->base.ops = &g_timebase_ops;
    fake->now_us = now_us;
}

void fake_timebase_set(fake_timebase_t *fake, uint64_t now_us)
{
    fake->now_us = now_us;
}

void fake_timebase_advance(fake_timebase_t *fake, uint64_t delta_us)
{
    fake->now_us += delta_us;
}
