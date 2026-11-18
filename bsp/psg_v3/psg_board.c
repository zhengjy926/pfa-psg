#include "bsp/psg_v3/psg_board.h"

#include "app/app_config.h"
#include "bsp/psg_v3/psg_board_config.h"
#include "stm32g4xx.h"

extern uint32_t SystemCoreClock;

static psg_board_devices_t g_devices;

static void clock_init(void)
{
    RCC->APB1ENR1 |= RCC_APB1ENR1_PWREN;
    (void)RCC->APB1ENR1;

    PWR->CR5 |= PWR_CR5_R1MODE;
    PWR->CR1 = (PWR->CR1 & ~PWR_CR1_VOS) | PWR_CR1_VOS_0;
    while ((PWR->SR2 & PWR_SR2_VOSF) != 0u) {
    }

    FLASH->ACR = (FLASH->ACR & ~FLASH_ACR_LATENCY) | FLASH_ACR_LATENCY_4WS | FLASH_ACR_PRFTEN |
                 FLASH_ACR_ICEN | FLASH_ACR_DCEN;

    RCC->CR |= RCC_CR_HSEON;
    while ((RCC->CR & RCC_CR_HSERDY) == 0u) {
    }

    /* HSE 8 MHz / M2 x N85 / R2 = 170 MHz; P7 gives 48.57 MHz to the ADCs
     * (PRM-702..PRM-708). */
    RCC->PLLCFGR = (RCC->PLLCFGR &
                    ~(RCC_PLLCFGR_PLLSRC | RCC_PLLCFGR_PLLM | RCC_PLLCFGR_PLLN | RCC_PLLCFGR_PLLP |
                      RCC_PLLCFGR_PLLQ | RCC_PLLCFGR_PLLR | RCC_PLLCFGR_PLLREN)) |
                   RCC_PLLCFGR_PLLSRC_1 | (1u << RCC_PLLCFGR_PLLM_Pos) |
                   (85u << RCC_PLLCFGR_PLLN_Pos) | RCC_PLLCFGR_PLLREN;
    RCC->CR |= RCC_CR_PLLON;
    while ((RCC->CR & RCC_CR_PLLRDY) == 0u) {
    }

    RCC->CFGR = (RCC->CFGR & ~(RCC_CFGR_SW | RCC_CFGR_HPRE | RCC_CFGR_PPRE1 | RCC_CFGR_PPRE2)) |
                RCC_CFGR_SW_PLL;
    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL) {
    }

    SystemCoreClock = PSG_MCU_CLOCK_HZ;
}

static void gpio_init(void)
{
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN | RCC_AHB2ENR_GPIOBEN | RCC_AHB2ENR_GPIOCEN |
                    RCC_AHB2ENR_GPIODEN | RCC_AHB2ENR_GPIOEEN | RCC_AHB2ENR_GPIOFEN |
                    RCC_AHB2ENR_GPIOGEN;
    (void)RCC->AHB2ENR;

    /* E-006: no rod conduction, both pulse outputs low, LED off (active low). */
    PSG_GPIO_LED_PORT->BSRR = (1u << PSG_GPIO_LED_PIN);
    PSG_GPIO_LED_PORT->MODER = (PSG_GPIO_LED_PORT->MODER & ~(0x3u << (PSG_GPIO_LED_PIN * 2u))) |
                               (0x1u << (PSG_GPIO_LED_PIN * 2u));

    PSG_GPIO_RUN_PORT->BSRR = (1u << (PSG_GPIO_RUN_PIN + 16u));
    PSG_GPIO_RUN_PORT->MODER = (PSG_GPIO_RUN_PORT->MODER & ~(0x3u << (PSG_GPIO_RUN_PIN * 2u))) |
                               (0x1u << (PSG_GPIO_RUN_PIN * 2u));

    GPIOD->BSRR = 0x3Fu << 24;
    GPIOD->MODER = (GPIOD->MODER & ~(0x3FFFu << 16)) | (0x5555u << 16);

    PSG_GPIO_LATCH_RESET_PORT->BSRR = (1u << (PSG_GPIO_LATCH_RESET_PIN + 16u));
    PSG_GPIO_LATCH_RESET_PORT->MODER =
        (PSG_GPIO_LATCH_RESET_PORT->MODER & ~(0x3u << (PSG_GPIO_LATCH_RESET_PIN * 2u))) |
        (0x1u << (PSG_GPIO_LATCH_RESET_PIN * 2u));

    GPIOG->MODER = (GPIOG->MODER & ~(0x3Fu << 14)) | (0x1u << (PSG_GPIO_LATCH_RESET_PIN * 2u));
    GPIOG->PUPDR = (GPIOG->PUPDR & ~(0xFu << 14)) | (0xAu << 14);

    GPIOA->BSRR = (1u << 24) | (1u << 25);
    GPIOA->MODER = (GPIOA->MODER & ~(0x3u << 16) & ~(0x3u << 18)) | (0x1u << 16) | (0x1u << 18);

    GPIOB->PUPDR = (GPIOB->PUPDR & ~(0x3u << 8) & ~(0x3u << 12)) | (0x2u << 8) | (0x2u << 12);
}

static void board_request_reset(void *context)
{
    (void)context;
    NVIC_SystemReset();
}

static bool board_overcurrent_active(void *context)
{
    (void)context;
    return comp_overcurrent(&g_devices.comp_pos.base) || comp_overcurrent(&g_devices.comp_neg.base);
}

static bool board_latch_active(void *context)
{
    (void)context;
    return comp_latch_active(&g_devices.comp_pos.base) || comp_latch_active(&g_devices.comp_neg.base);
}

static void board_latch_reset_pulse(void *context)
{
    (void)context;
    comp_latch_reset_pulse(&g_devices.comp_pos.base);
}

static void board_set_running_indicator(void *context, bool on)
{
    (void)context;
    if (on) {
        PSG_GPIO_RUN_PORT->BSRR = (1u << PSG_GPIO_RUN_PIN);
    } else {
        PSG_GPIO_RUN_PORT->BSRR = (1u << (PSG_GPIO_RUN_PIN + 16u));
    }
}

static const psg_board_ops_t g_board_ops = {
    .request_reset = board_request_reset,
    .overcurrent_active = board_overcurrent_active,
    .latch_active = board_latch_active,
    .latch_reset_pulse = board_latch_reset_pulse,
    .set_running_indicator = board_set_running_indicator,
};

void psg_board_init(void)
{
    NVIC_SetPriorityGrouping(3u);
    clock_init();
    gpio_init();

    stm32_timebase_init(&g_devices.timebase);
    stm32_led_init(&g_devices.led);
    stm32_flash_init(&g_devices.flash);
    stm32_comp_init(&g_devices.comp_pos, 0u);
    stm32_comp_init(&g_devices.comp_neg, 1u);
    stm32_iwdg_init(&g_devices.iwdg);
    stm32_watchdog_init(&g_devices.watchdog, &g_devices.iwdg.base);
    stm32_uart_init(&g_devices.uart);

    g_devices.board.ops = &g_board_ops;
    g_devices.board.context = &g_devices;
}

psg_board_devices_t *psg_board_devices(void)
{
    return &g_devices;
}
