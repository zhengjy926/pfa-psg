/*
 * Board indicator LED device interface (F-015, PRM-606).
 */
#ifndef PSG_DRV_LED_H
#define PSG_DRV_LED_H

#include <stdbool.h>

typedef struct led led_t;

typedef struct {
    void (*set)(const led_t *led, bool on);
} led_ops_t;

struct led {
    const led_ops_t *ops;
};

static inline void led_set(const led_t *led, bool on)
{
    led->ops->set(led, on);
}

#endif /* PSG_DRV_LED_H */
