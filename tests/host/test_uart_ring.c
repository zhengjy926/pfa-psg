#include "test_support.h"

#include <string.h>

#include "drv/uart/uart_ring.h"

TEST(uart_ring, pops_in_order_with_receive_timestamps)
{
    uart_data_t data;
    uint8_t values[4];
    uint32_t stamps[4];

    (void)memset(&data, 0, sizeof(data));
    uart_ring_push(&data, 0xAAu, 10u);
    uart_ring_push(&data, 0xBBu, 20u);

    CHECK_EQ(uart_ring_pop(&data, values, stamps, 4u), 2u);
    CHECK_EQ(values[0], 0xAAu);
    CHECK_EQ(stamps[0], 10u);
    CHECK_EQ(values[1], 0xBBu);
    CHECK_EQ(stamps[1], 20u);
    CHECK_EQ(uart_ring_pop(&data, values, stamps, 4u), 0u);
}

TEST(uart_ring, overflow_drops_oldest_and_preserves_surviving_timestamps)
{
    uart_data_t data;
    uint8_t values[UART_RX_RING_BYTES];
    uint32_t stamps[UART_RX_RING_BYTES];
    size_t index;
    size_t count;

    (void)memset(&data, 0, sizeof(data));
    for (index = 0u; index < UART_RX_RING_BYTES; index++) {
        uart_ring_push(&data, (uint8_t)(index & 0xFFu), (uint32_t)index);
    }

    count = uart_ring_pop(&data, values, stamps, UART_RX_RING_BYTES);
    CHECK_EQ(count, UART_RX_RING_BYTES - 1u);
    CHECK_EQ(values[0], 0x01u);
    CHECK_EQ(stamps[0], 1u);
    CHECK_EQ(values[count - 1u], (uint8_t)((UART_RX_RING_BYTES - 1u) & 0xFFu));
    CHECK_EQ(stamps[count - 1u], UART_RX_RING_BYTES - 1u);
    CHECK_EQ(uart_ring_take_gaps(&data), 1u);
    CHECK_EQ(uart_ring_take_gaps(&data), 0u);
}

TEST(uart_ring, transmit_completion_is_reported_once)
{
    uart_data_t data;
    static const uint8_t frame[] = {0xFAu, 0x09u};
    uint64_t completion = 0u;

    (void)memset(&data, 0, sizeof(data));
    uart_tx_begin(&data, frame, sizeof(frame));
    CHECK(uart_tx_is_busy(&data));
    CHECK(!uart_tx_poll_complete(&data, &completion));

    uart_tx_mark_complete(&data, 1234u);
    CHECK(!uart_tx_is_busy(&data));
    CHECK(uart_tx_poll_complete(&data, &completion));
    CHECK_EQ(completion, 1234u);
    CHECK(!uart_tx_poll_complete(&data, &completion));
}
