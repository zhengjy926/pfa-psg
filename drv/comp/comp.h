/*
 * Comparator over-current detection and independent latch input (E-001,
 * E-010).  The latch is a fixed hardware path at PRM-406 and cannot be
 * configured or masked by firmware.
 */
#ifndef PSG_DRV_COMP_H
#define PSG_DRV_COMP_H

#include <stdbool.h>

typedef struct comp comp_t;

typedef struct {
    bool (*overcurrent)(const comp_t *comp);
    bool (*latch_active)(const comp_t *comp);
    void (*latch_reset_pulse)(const comp_t *comp);
} comp_ops_t;

struct comp {
    const comp_ops_t *ops;
};

static inline bool comp_overcurrent(const comp_t *comp)
{
    return comp->ops->overcurrent(comp);
}

static inline bool comp_latch_active(const comp_t *comp)
{
    return comp->ops->latch_active(comp);
}

static inline void comp_latch_reset_pulse(const comp_t *comp)
{
    comp->ops->latch_reset_pulse(comp);
}

#endif /* PSG_DRV_COMP_H */
