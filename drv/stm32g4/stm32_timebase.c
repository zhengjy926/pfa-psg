#include "drv/stm32g4/stm32_timebase.h"

#include "bsp/psg_v3/psg_board_config.h"
#include "stm32g4xx.h"

#define TIM2_PRESCALER ((PSG_MCU_CLOCK_HZ / 1000000u) - 1u)

static uint64_t now_us(const timebase_t *base)
{
    stm32_timebase_t *impl = (stm32_timebase_t *)(void *)base;
    uint32_t counter = TIM2->CNT;
    uint32_t delta = counter - impl->last_counter;

    impl->last_counter = counter;
    impl->accumulated_us += (uint64_t)delta;
    return impl->accumulated_us;
}

static const timebase_ops_t g_timebase_ops = {
    .now_us = now_us,
};

void stm32_timebase_init(stm32_timebase_t *impl)
{
    RCC->APB1ENR1 |= RCC_APB1ENR1_TIM2EN;
    (void)RCC->APB1ENR1;

    TIM2->CR1 = 0u;
    TIM2->PSC = (uint32_t)TIM2_PRESCALER;
    TIM2->ARR = 0xFFFFFFFFu;
    TIM2->EGR = TIM_EGR_UG;
    TIM2->SR = 0u;
    TIM2->CR1 = TIM_CR1_CEN;

    impl->base.ops = &g_timebase_ops;
    impl->accumulated_us = 0u;
    impl->last_counter = TIM2->CNT;
}

uint32_t stm32_timebase_counter(void)
{
    return TIM2->CNT;
}
