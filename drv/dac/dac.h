/*
 * DAC voltage-step device interface (F-024, I-016).
 */
#ifndef PSG_DRV_DAC_H
#define PSG_DRV_DAC_H

#include <stdbool.h>
#include <stdint.h>

typedef struct dac dac_t;

typedef struct {
    bool (*set_millivolts)(const dac_t *dac, uint16_t millivolts);
} dac_ops_t;

struct dac {
    const dac_ops_t *ops;
};

static inline bool dac_set_millivolts(const dac_t *dac, uint16_t millivolts)
{
    return dac->ops->set_millivolts(dac, millivolts);
}

#endif /* PSG_DRV_DAC_H */
