/*
 * PSG v3 board: static device instances and power-up assembly (architecture
 * section 5).  E-006 requires all outputs to stay inactive until init.
 */
#ifndef PSG_BSP_PSG_V3_PSG_BOARD_H
#define PSG_BSP_PSG_V3_PSG_BOARD_H

#include "bsp/psg_board.h"
#include "drv/stm32g4/stm32_comp.h"
#include "drv/stm32g4/stm32_flash.h"
#include "drv/stm32g4/stm32_iwdg.h"
#include "drv/stm32g4/stm32_led.h"
#include "drv/stm32g4/stm32_timebase.h"
#include "drv/stm32g4/stm32_uart.h"
#include "drv/stm32g4/stm32_watchdog.h"

typedef struct {
    psg_board_t board;
    stm32_timebase_t timebase;
    stm32_uart_t uart;
    stm32_led_t led;
    stm32_flash_t flash;
    stm32_comp_t comp_pos;
    stm32_comp_t comp_neg;
    stm32_iwdg_t iwdg;
    stm32_watchdog_t watchdog;
} psg_board_devices_t;

void psg_board_init(void);
psg_board_devices_t *psg_board_devices(void);

#endif /* PSG_BSP_PSG_V3_PSG_BOARD_H */
