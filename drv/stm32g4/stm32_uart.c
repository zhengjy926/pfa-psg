/*
 * STM32G4 USART + DMA driver for the ECSP protocol port (I-001, I-025).
 *
 * RX: circular DMA into a 1 KiB buffer; IDLE, half-transfer and transfer-
 * complete interrupts fold the newly written bytes into the timestamped ring.
 * TX: one DMA frame in flight; USART TC (not the DMA completion) marks the
 * line completion used for heartbeat scheduling and the 0x07 reset ordering.
 *
 * The receive timestamp is sampled once per interrupt for all bytes in that
 * interval.  The bench must confirm the resulting error band against the line
 * (HW-05); protocol decisions only rely on PRM-115.
 */
#include "drv/stm32g4/stm32_uart.h"

#include "shared/psg_config.h"
#include "bsp/psg_v3/psg_board_config.h"
#include "drv/stm32g4/stm32_timebase.h"
#include "drv/uart/uart_ring.h"
#include "stm32g4xx.h"

#if PSG_PROTO_UART_INSTANCE == 2
#define PSG_USART USART2
#define PSG_USART_IRQ_HANDLER USART2_IRQHandler
#define PSG_USART_IRQn USART2_IRQn
#define PSG_RX_DMA DMA1_Channel3
#define PSG_TX_DMA DMA1_Channel4
#define PSG_RX_DMAMUX DMAMUX1_Channel3
#define PSG_TX_DMAMUX DMAMUX1_Channel4
#define PSG_RX_DMA_IRQ_HANDLER DMA1_Channel3_IRQHandler
#define PSG_TX_DMA_IRQ_HANDLER DMA1_Channel4_IRQHandler
#define PSG_RX_DMA_IRQn DMA1_Channel3_IRQn
#define PSG_TX_DMA_IRQn DMA1_Channel4_IRQn
#define PSG_RX_REQUEST 26u
#define PSG_TX_REQUEST 27u
#define PSG_RX_FLAGS (DMA_IFCR_CGIF3 | DMA_IFCR_CTCIF3 | DMA_IFCR_CHTIF3 | DMA_IFCR_CTEIF3)
#define PSG_TX_FLAGS (DMA_IFCR_CGIF4 | DMA_IFCR_CTCIF4 | DMA_IFCR_CHTIF4 | DMA_IFCR_CTEIF4)
#define PSG_RX_TC_FLAG DMA_ISR_TCIF3
#define PSG_RX_HT_FLAG DMA_ISR_HTIF3
#define PSG_TX_TC_FLAG DMA_ISR_TCIF4
#else
#define PSG_USART USART1
#define PSG_USART_IRQ_HANDLER USART1_IRQHandler
#define PSG_USART_IRQn USART1_IRQn
#define PSG_RX_DMA DMA1_Channel1
#define PSG_TX_DMA DMA1_Channel2
#define PSG_RX_DMAMUX DMAMUX1_Channel1
#define PSG_TX_DMAMUX DMAMUX1_Channel2
#define PSG_RX_DMA_IRQ_HANDLER DMA1_Channel1_IRQHandler
#define PSG_TX_DMA_IRQ_HANDLER DMA1_Channel2_IRQHandler
#define PSG_RX_DMA_IRQn DMA1_Channel1_IRQn
#define PSG_TX_DMA_IRQn DMA1_Channel2_IRQn
#define PSG_RX_REQUEST 24u
#define PSG_TX_REQUEST 25u
#define PSG_RX_FLAGS (DMA_IFCR_CGIF1 | DMA_IFCR_CTCIF1 | DMA_IFCR_CHTIF1 | DMA_IFCR_CTEIF1)
#define PSG_TX_FLAGS (DMA_IFCR_CGIF2 | DMA_IFCR_CTCIF2 | DMA_IFCR_CHTIF2 | DMA_IFCR_CTEIF2)
#define PSG_RX_TC_FLAG DMA_ISR_TCIF1
#define PSG_RX_HT_FLAG DMA_ISR_HTIF1
#define PSG_TX_TC_FLAG DMA_ISR_TCIF2
#endif

#define RX_DMA_BYTES UART_RX_RING_BYTES
#define RX_DMA_MASK (RX_DMA_BYTES - 1u)

static stm32_uart_t *g_uart;

void PSG_USART_IRQ_HANDLER(void);
void PSG_RX_DMA_IRQ_HANDLER(void);
void PSG_TX_DMA_IRQ_HANDLER(void);

static void rx_progress(stm32_uart_t *impl)
{
    uint32_t position = (RX_DMA_BYTES - PSG_RX_DMA->CNDTR) & RX_DMA_MASK;
    uint32_t delta = (position - impl->rx_last_pos) & RX_DMA_MASK;
    uint32_t timestamp = stm32_timebase_counter();
    uint32_t index;

    for (index = 0u; index < delta; index++) {
        uint32_t offset = (impl->rx_last_pos + index) & RX_DMA_MASK;
        uart_ring_push(&impl->data, impl->rx_buffer[offset], timestamp);
    }
    impl->rx_last_pos = position;
}

static size_t uart_read_impl(void *context, uint8_t *values, uint32_t *timestamps, size_t capacity)
{
    stm32_uart_t *impl = (stm32_uart_t *)context;

    return uart_ring_pop(&impl->data, values, timestamps, capacity);
}

static uint32_t uart_take_gaps_impl(void *context)
{
    stm32_uart_t *impl = (stm32_uart_t *)context;

    return uart_ring_take_gaps(&impl->data);
}

static bool uart_write_impl(void *context, const uint8_t *frame, size_t length)
{
    stm32_uart_t *impl = (stm32_uart_t *)context;

    if (uart_tx_is_busy(&impl->data) || (length == 0u) || (length > 0xFFFFu)) {
        return false;
    }

    PSG_TX_DMA->CCR &= ~DMA_CCR_EN;
    DMA1->IFCR = impl->tx_flags_clear;
    PSG_TX_DMA->CPAR = (uint32_t)(uintptr_t)&PSG_USART->TDR;
    PSG_TX_DMA->CMAR = (uint32_t)(uintptr_t)frame;
    PSG_TX_DMA->CNDTR = (uint32_t)length;
    PSG_TX_DMA->CCR |= DMA_CCR_EN;

    uart_tx_begin(&impl->data, frame, (uint32_t)length);
    return true;
}

static bool uart_poll_complete_impl(void *context, uint64_t *completion_us)
{
    stm32_uart_t *impl = (stm32_uart_t *)context;

    return uart_tx_poll_complete(&impl->data, completion_us);
}

static bool uart_busy_impl(const void *context)
{
    const stm32_uart_t *impl = (const stm32_uart_t *)context;

    return uart_tx_is_busy(&impl->data);
}

static const uart_ops_t g_uart_ops = {
    .read = uart_read_impl,
    .take_gaps = uart_take_gaps_impl,
    .write = uart_write_impl,
    .poll_tx_complete = uart_poll_complete_impl,
    .tx_busy = uart_busy_impl,
};

static void gpio_init(void)
{
#if PSG_PROTO_UART_INSTANCE == 2
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;
    (void)RCC->AHB2ENR;
    GPIOA->MODER = (GPIOA->MODER & ~((0x3u << 4) | (0x3u << 6))) | (0x2u << 4) | (0x2u << 6);
    GPIOA->OSPEEDR |= (0x2u << 4) | (0x2u << 6);
    GPIOA->AFR[0] = (GPIOA->AFR[0] & ~((0xFu << 8) | (0xFu << 12))) | (0x7u << 8) | (0x7u << 12);
    GPIOA->PUPDR = (GPIOA->PUPDR & ~(0x3u << 6)) | (0x1u << 6); /* RX pull-up */
#else
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOCEN;
    (void)RCC->AHB2ENR;
    GPIOC->MODER = (GPIOC->MODER & ~((0x3u << 8) | (0x3u << 10))) | (0x2u << 8) | (0x2u << 10);
    GPIOC->OSPEEDR |= (0x2u << 8) | (0x2u << 10);
    GPIOC->AFR[0] = (GPIOC->AFR[0] & ~((0xFu << 16) | (0xFu << 20))) | (0x7u << 16) | (0x7u << 20);
    GPIOC->PUPDR = (GPIOC->PUPDR & ~(0x3u << 20)) | (0x1u << 20);
#endif
}

void stm32_uart_init(stm32_uart_t *impl)
{
    impl->config.name = (PSG_PROTO_UART_INSTANCE == 2) ? "usart2" : "usart1";
    impl->config.instance = (uint32_t)PSG_PROTO_UART_INSTANCE;
    impl->config.irq_priority = (uint8_t)PSG_IRQ_PRIO_COMMUNICATION;
    impl->config.dma_rx = true;
    impl->instance = PSG_USART;
    impl->rx_dma = PSG_RX_DMA;
    impl->tx_dma = PSG_TX_DMA;
    impl->rx_dmamux = PSG_RX_DMAMUX;
    impl->tx_dmamux = PSG_TX_DMAMUX;
    impl->rx_last_pos = 0u;
    impl->rx_flags_clear = PSG_RX_FLAGS;
    impl->tx_flags_clear = PSG_TX_FLAGS;
    impl->base.config = &impl->config;
    impl->base.data = &impl->data;
    impl->base.context = impl;
    impl->base.ops = &g_uart_ops;
    g_uart = impl;

    gpio_init();

    RCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN | RCC_AHB1ENR_DMAMUX1EN;
#if PSG_PROTO_UART_INSTANCE == 2
    RCC->APB1ENR1 |= RCC_APB1ENR1_USART2EN;
#else
    RCC->APB2ENR |= RCC_APB2ENR_USART1EN;
#endif
    (void)RCC->APB1ENR1;

    PSG_USART->CR1 = 0u;
    PSG_USART->BRR = (uint16_t)((PSG_MCU_CLOCK_HZ + (PSG_UART_BAUD / 2u)) / PSG_UART_BAUD);
    PSG_USART->CR3 = USART_CR3_EIE | USART_CR3_DMAR | USART_CR3_DMAT;

    PSG_RX_DMAMUX->CCR = PSG_RX_REQUEST;
    PSG_RX_DMA->CCR = 0u;
    PSG_RX_DMA->CPAR = (uint32_t)(uintptr_t)&PSG_USART->RDR;
    PSG_RX_DMA->CMAR = (uint32_t)(uintptr_t)impl->rx_buffer;
    PSG_RX_DMA->CNDTR = RX_DMA_BYTES;
    PSG_RX_DMA->CCR = DMA_CCR_MINC | DMA_CCR_CIRC | DMA_CCR_HTIE | DMA_CCR_TCIE | DMA_CCR_EN;

    PSG_TX_DMAMUX->CCR = PSG_TX_REQUEST;
    PSG_TX_DMA->CCR = DMA_CCR_MINC | DMA_CCR_DIR | DMA_CCR_TCIE;

    /* Enable the USART only after the receive DMA is armed. */
    PSG_USART->CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE | USART_CR1_FIFOEN | USART_CR1_IDLEIE;

    NVIC_SetPriority(PSG_USART_IRQn, (uint32_t)PSG_IRQ_PRIO_COMMUNICATION);
    NVIC_SetPriority(PSG_RX_DMA_IRQn, (uint32_t)PSG_IRQ_PRIO_COMMUNICATION);
    NVIC_SetPriority(PSG_TX_DMA_IRQn, (uint32_t)PSG_IRQ_PRIO_COMMUNICATION);
    NVIC_EnableIRQ(PSG_USART_IRQn);
    NVIC_EnableIRQ(PSG_RX_DMA_IRQn);
    NVIC_EnableIRQ(PSG_TX_DMA_IRQn);

    impl->data.initialized = true;
}

void PSG_USART_IRQ_HANDLER(void)
{
    stm32_uart_t *impl = g_uart;
    uint32_t isr = PSG_USART->ISR;

    if (impl == NULL) {
        return;
    }

    if ((isr & USART_ISR_IDLE) != 0u) {
        PSG_USART->ICR = USART_ICR_IDLECF;
        rx_progress(impl);
    }
    if ((isr & (USART_ISR_ORE | USART_ISR_FE | USART_ISR_NE | USART_ISR_PE)) != 0u) {
        PSG_USART->ICR = USART_ICR_ORECF | USART_ICR_FECF | USART_ICR_NECF | USART_ICR_PECF;
        impl->data.gaps++;
    }
    if (((isr & USART_ISR_TC) != 0u) && ((PSG_USART->CR1 & USART_CR1_TCIE) != 0u)) {
        PSG_USART->ICR = USART_ICR_TCCF;
        PSG_USART->CR1 &= ~USART_CR1_TCIE;
        uart_tx_mark_complete(&impl->data, stm32_timebase_counter());
    }
}

void PSG_RX_DMA_IRQ_HANDLER(void)
{
    stm32_uart_t *impl = g_uart;

    if (impl == NULL) {
        return;
    }
    if ((DMA1->ISR & (PSG_RX_TC_FLAG | PSG_RX_HT_FLAG)) != 0u) {
        rx_progress(impl);
        DMA1->IFCR = impl->rx_flags_clear;
    }
}

void PSG_TX_DMA_IRQ_HANDLER(void)
{
    stm32_uart_t *impl = g_uart;

    if (impl == NULL) {
        return;
    }
    if ((DMA1->ISR & PSG_TX_TC_FLAG) != 0u) {
        DMA1->IFCR = impl->tx_flags_clear;
        PSG_TX_DMA->CCR &= ~DMA_CCR_EN;
        PSG_USART->ICR = USART_ICR_TCCF;
        PSG_USART->CR1 |= USART_CR1_TCIE;
    }
}
