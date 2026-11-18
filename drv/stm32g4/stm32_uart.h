#ifndef PSG_DRV_STM32G4_STM32_UART_H
#define PSG_DRV_STM32G4_STM32_UART_H

#include <stdint.h>

#include "drv/uart/uart.h"

typedef struct {
    uart_t base;
    uart_data_t data;
    uart_config_t config;
    void *instance;
    void *rx_dma;
    void *tx_dma;
    void *rx_dmamux;
    void *tx_dmamux;
    uint8_t rx_buffer[UART_RX_RING_BYTES];
    volatile uint32_t rx_last_pos;
    uint32_t rx_flags_clear;
    uint32_t tx_flags_clear;
} stm32_uart_t;

void stm32_uart_init(stm32_uart_t *impl);

#endif /* PSG_DRV_STM32G4_STM32_UART_H */
