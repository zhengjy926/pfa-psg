/*
 * Independent watchdog hardware interface (PRM-718..PRM-720, E-008).
 */
#ifndef PSG_DRV_IWDG_H
#define PSG_DRV_IWDG_H

#include <stdbool.h>
#include <stdint.h>

typedef struct iwdg iwdg_t;

typedef struct {
    bool (*start)(void *context);
    void (*refresh)(void *context);
    uint32_t (*reset_cause)(const void *context);
} iwdg_ops_t;

struct iwdg {
    const iwdg_ops_t *ops;
    void *context;
};

static inline bool iwdg_start(const iwdg_t *iwdg)
{
    return iwdg->ops->start(iwdg->context);
}

static inline void iwdg_refresh(const iwdg_t *iwdg)
{
    iwdg->ops->refresh(iwdg->context);
}

static inline uint32_t iwdg_reset_cause(const iwdg_t *iwdg)
{
    return iwdg->ops->reset_cause(iwdg->context);
}

#endif /* PSG_DRV_IWDG_H */
