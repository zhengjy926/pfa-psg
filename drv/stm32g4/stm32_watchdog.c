#include "drv/stm32g4/stm32_watchdog.h"

static bool watchdog_start_impl(const watchdog_t *watchdog)
{
    const stm32_watchdog_t *impl = (const stm32_watchdog_t *)(const void *)watchdog;

    return iwdg_start(impl->iwdg);
}

static void watchdog_refresh_impl(const watchdog_t *watchdog)
{
    const stm32_watchdog_t *impl = (const stm32_watchdog_t *)(const void *)watchdog;

    iwdg_refresh(impl->iwdg);
}

static uint32_t watchdog_reset_cause_impl(const watchdog_t *watchdog)
{
    const stm32_watchdog_t *impl = (const stm32_watchdog_t *)(const void *)watchdog;

    return iwdg_reset_cause(impl->iwdg);
}

static const watchdog_ops_t g_watchdog_ops = {
    .start = watchdog_start_impl,
    .refresh = watchdog_refresh_impl,
    .reset_cause = watchdog_reset_cause_impl,
};

void stm32_watchdog_init(stm32_watchdog_t *impl, const iwdg_t *iwdg)
{
    impl->base.ops = &g_watchdog_ops;
    impl->iwdg = iwdg;
}
