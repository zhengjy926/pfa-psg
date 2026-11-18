/*
 * Watchdog refresh service (E-008).
 *
 * Only the task monitor calls this, and only after every supervised task has
 * proved progress within its maximum interval.
 */
#ifndef PSG_DRV_WATCHDOG_H
#define PSG_DRV_WATCHDOG_H

#include <stdbool.h>
#include <stdint.h>

typedef struct watchdog watchdog_t;

typedef struct {
    bool (*start)(const watchdog_t *watchdog);
    void (*refresh)(const watchdog_t *watchdog);
    uint32_t (*reset_cause)(const watchdog_t *watchdog);
} watchdog_ops_t;

struct watchdog {
    const watchdog_ops_t *ops;
};

static inline bool watchdog_start(const watchdog_t *watchdog)
{
    return watchdog->ops->start(watchdog);
}

static inline void watchdog_refresh(const watchdog_t *watchdog)
{
    watchdog->ops->refresh(watchdog);
}

static inline uint32_t watchdog_reset_cause(const watchdog_t *watchdog)
{
    return watchdog->ops->reset_cause(watchdog);
}

#endif /* PSG_DRV_WATCHDOG_H */
