#include "drv/stm32g4/stm32_iwdg.h"

#include "stm32g4xx.h"

#define IWDG_KEY_ENABLE 0xCCCCu
#define IWDG_KEY_REFRESH 0xAAAAu
#define IWDG_KEY_WRITE 0x5555u
#define IWDG_PRESCALER_DIV8 1u
#define IWDG_RELOAD 3999u

static bool iwdg_start_impl(void *context)
{
    (void)context;
    IWDG->KR = IWDG_KEY_ENABLE;
    IWDG->KR = IWDG_KEY_WRITE;
    IWDG->PR = IWDG_PRESCALER_DIV8;
    IWDG->RLR = IWDG_RELOAD;
    while (IWDG->SR != 0u) {
    }
    IWDG->KR = IWDG_KEY_REFRESH;
    return true;
}

static void iwdg_refresh_impl(void *context)
{
    (void)context;
    IWDG->KR = IWDG_KEY_REFRESH;
}

static uint32_t iwdg_reset_cause_impl(const void *context)
{
    uint32_t csr = RCC->CSR;
    (void)context;

    RCC->CSR |= RCC_CSR_RMVF;
    return csr;
}

static const iwdg_ops_t g_iwdg_ops = {
    .start = iwdg_start_impl,
    .refresh = iwdg_refresh_impl,
    .reset_cause = iwdg_reset_cause_impl,
};

void stm32_iwdg_init(stm32_iwdg_t *impl)
{
    impl->base.ops = &g_iwdg_ops;
    impl->base.context = impl;
}
