/*
 * Comparator/latch GPIO evidence and latch reset pulse (E-001..E-003, E-010).
 *
 * The analog comparators and their HRTIM fault routing arrive with the
 * over-current ticket; this driver exposes the independent latch inputs and
 * the PG9 reset line so the 0x3D flow can run against real hardware.
 */
#include "drv/stm32g4/stm32_comp.h"

#include "bsp/psg_v3/psg_board_config.h"
#include "drv/stm32g4/stm32_timebase.h"
#include "stm32g4xx.h"

static bool comp_overcurrent_impl(const comp_t *comp)
{
    (void)comp;
    return false;
}

static bool comp_latch_active_impl(const comp_t *comp)
{
    const stm32_comp_t *impl = (const stm32_comp_t *)(const void *)comp;

    if (impl->channel == 0u) {
        return (PSG_GPIO_LATCH_POS_PORT->IDR & (1u << PSG_GPIO_LATCH_POS_PIN)) != 0u;
    }
    return (PSG_GPIO_LATCH_NEG_PORT->IDR & (1u << PSG_GPIO_LATCH_NEG_PIN)) != 0u;
}

static void comp_latch_reset_pulse_impl(const comp_t *comp)
{
    uint32_t start;
    (void)comp;

    PSG_GPIO_LATCH_RESET_PORT->BSRR = (1u << PSG_GPIO_LATCH_RESET_PIN);
    start = stm32_timebase_counter();
    while ((stm32_timebase_counter() - start) < 2u) {
    }
    PSG_GPIO_LATCH_RESET_PORT->BSRR = (1u << (PSG_GPIO_LATCH_RESET_PIN + 16u));
}

static const comp_ops_t g_comp_ops = {
    .overcurrent = comp_overcurrent_impl,
    .latch_active = comp_latch_active_impl,
    .latch_reset_pulse = comp_latch_reset_pulse_impl,
};

void stm32_comp_init(stm32_comp_t *impl, uint8_t channel)
{
    impl->base.ops = &g_comp_ops;
    impl->channel = channel;
}
