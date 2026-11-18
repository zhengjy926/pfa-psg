#include "fakes/fake_led.h"

static void fake_led_set(const led_t *led, bool on)
{
    fake_led_t *fake = (fake_led_t *)(void *)led;

    if (fake->on != on) {
        fake->transitions++;
    }
    fake->on = on;
}

static const led_ops_t g_led_ops = {
    .set = fake_led_set,
};

void fake_led_init(fake_led_t *fake)
{
    fake->base.ops = &g_led_ops;
    fake->on = false;
    fake->transitions = 0u;
}
