/*
 * ADC capture device interface (F-035, F-037).
 *
 * The fixed-size circular buffer belongs to the BSP.  Half/full transfer
 * interrupts are data-ready hints only; sequence boundaries come from the
 * TIMA repeat event and an NDTR snapshot (PSG-13).
 */
#ifndef PSG_DRV_ADC_CAPTURE_H
#define PSG_DRV_ADC_CAPTURE_H

#include <stdbool.h>
#include <stdint.h>

typedef struct adc_capture adc_capture_t;

typedef struct {
    bool (*start)(void *context);
    void (*stop)(void *context);
    bool (*take_ready)(void *context, uint8_t *half_mask);
    uint32_t (*write_progress)(void *context);
    bool (*error)(const void *context);
} adc_capture_ops_t;

struct adc_capture {
    const adc_capture_ops_t *ops;
    void *context;
};

static inline bool adc_capture_start(const adc_capture_t *capture)
{
    return capture->ops->start(capture->context);
}

static inline void adc_capture_stop(const adc_capture_t *capture)
{
    capture->ops->stop(capture->context);
}

static inline bool adc_capture_take_ready(const adc_capture_t *capture, uint8_t *half_mask)
{
    return capture->ops->take_ready(capture->context, half_mask);
}

static inline uint32_t adc_capture_write_progress(const adc_capture_t *capture)
{
    return capture->ops->write_progress(capture->context);
}

static inline bool adc_capture_error(const adc_capture_t *capture)
{
    return capture->ops->error(capture->context);
}

#endif /* PSG_DRV_ADC_CAPTURE_H */
