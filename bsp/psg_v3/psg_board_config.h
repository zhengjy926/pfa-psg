/*
 * Board configuration for PSG v3 (STM32G474QET6, LQFP128).
 * Pin assignment, peripheral instances, NVIC priorities, Flash regions and
 * the FreeRTOS interrupt threshold live here (architecture section 5).
 */
#ifndef PSG_BSP_PSG_V3_PSG_BOARD_CONFIG_H
#define PSG_BSP_PSG_V3_PSG_BOARD_CONFIG_H

#include <stdint.h>

#define PSG_MCU_CLOCK_HZ 170000000u
#define PSG_HSE_CLOCK_HZ 8000000u

/* NVIC priorities (PRM-727..PRM-730). */
#define PSG_IRQ_PRIO_PROTECTION 4u
#define PSG_IRQ_PRIO_COMMUNICATION 5u
#define PSG_IRQ_PRIO_SYSTICK 15u

/* Flash layout (ADR 0002): bank1 holds code/constants, bank2 holds records. */
#define PSG_FLASH_BANK1_BASE 0x08000000u
#define PSG_FLASH_BANK1_BYTES (256u * 1024u)
#define PSG_FLASH_BANK2_BASE 0x08040000u
#define PSG_FLASH_RECORD_BASE PSG_FLASH_BANK2_BASE
#define PSG_FLASH_RECORD_BYTES (256u * 1024u)
#define PSG_FLASH_PAGE_BYTES 2048u

/* Pin bindings used by this increment (SRS section 7 / MCU map). */
#define PSG_GPIO_LED_PORT GPIOB
#define PSG_GPIO_LED_PIN 9u
#define PSG_GPIO_RUN_PORT GPIOB
#define PSG_GPIO_RUN_PIN 5u
#define PSG_GPIO_LATCH_NEG_PORT GPIOG
#define PSG_GPIO_LATCH_NEG_PIN 7u
#define PSG_GPIO_LATCH_POS_PORT GPIOG
#define PSG_GPIO_LATCH_POS_PIN 8u
#define PSG_GPIO_LATCH_RESET_PORT GPIOG
#define PSG_GPIO_LATCH_RESET_PIN 9u

#endif /* PSG_BSP_PSG_V3_PSG_BOARD_CONFIG_H */
