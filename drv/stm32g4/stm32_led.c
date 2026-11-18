#include "drv/stm32g4/stm32_led.h"

#include "bsp/psg_v3/psg_board_config.h"
#include "stm32g4xx.h"

static void led_set_impl(const led_t *led, bool on)
{
    (void)led;
    if (on) {
        PSG_GPIO_LED_PORT->BSRR = (1u << PSG_GPIO_LED_PIN); /* active low: drive 0 */
    } else {
        PSG_GPIO_LED_PORT->BSRR = (1u << (PSG_GPIO_LED_PIN + 16u));
    }
}

static const led_ops_t g_led_ops = {
    .set = led_set_impl,
};

void stm32_led_init(stm32_led_t *impl)
{
    impl->base.ops = &g_led_ops;
}
