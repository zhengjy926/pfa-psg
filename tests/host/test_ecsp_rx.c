#include "test_support.h"

#include <string.h>

#include "app/ecsp/ecsp_rx.h"

static void feed(ecsp_rx_t *rx, const uint8_t *bytes, size_t count, uint32_t timestamp_us)
{
    uint8_t values[64];
    uint32_t stamps[64];
    size_t offset = 0u;

    while (offset < count) {
        size_t chunk = count - offset;
        size_t index;

        if (chunk > (sizeof(values) / sizeof(values[0]))) {
            chunk = sizeof(values) / sizeof(values[0]);
        }
        for (index = 0u; index < chunk; index++) {
            values[index] = bytes[offset + index];
            stamps[index] = timestamp_us;
        }
        (void)ecsp_rx_feed(rx, values, stamps, chunk);
        offset += chunk;
    }
}

static void feed_byte(ecsp_rx_t *rx, uint8_t value, uint32_t timestamp_us)
{
    (void)ecsp_rx_feed(rx, &value, &timestamp_us, 1u);
}

TEST(ecsp_rx, parses_a_complete_in_time_frame)
{
    static const uint8_t raw[] = {0xFAu, 0x09u, 0x00u, 0x03u, 0x12u, 0x02u, 0x85u, 0x60u, 0x0Du};
    ecsp_rx_t rx;
    ecsp_frame_t frame;

    ecsp_rx_init(&rx);
    feed(&rx, raw, sizeof(raw), 0u);
    CHECK(ecsp_rx_poll(&rx, 0u, &frame));
    CHECK_EQ(frame.command, 0x12u);
    CHECK_EQ(frame.payload_length, 0u);
    CHECK(!ecsp_rx_poll(&rx, 0u, &frame));
}

TEST(ecsp_rx, resynchronises_past_a_bad_crc_to_the_next_frame)
{
    static const uint8_t bad[] = {0xFAu, 0x0Au, 0x00u, 0x03u, 0x32u, 0x02u, 0x01u, 0x61u, 0x9Au, 0x0Du};
    static const uint8_t good[] = {0xFAu, 0x09u, 0x00u, 0x03u, 0x12u, 0x02u, 0x85u, 0x60u, 0x0Du};
    uint8_t stream[sizeof(bad) + sizeof(good)];
    ecsp_rx_t rx;
    ecsp_frame_t frame;

    (void)memcpy(stream, bad, sizeof(bad));
    (void)memcpy(&stream[sizeof(bad)], good, sizeof(good));
    stream[6] ^= 0x01u; /* corrupt the first frame's payload */

    ecsp_rx_init(&rx);
    feed(&rx, stream, sizeof(stream), 0u);
    CHECK(ecsp_rx_poll(&rx, 0u, &frame));
    CHECK_EQ(frame.command, 0x12u);
}

TEST(ecsp_rx, rejects_out_of_range_length_immediately_and_rescans)
{
    static const uint8_t prefix[] = {0xFAu, 0x08u, 0x00u};
    static const uint8_t good[] = {0xFAu, 0x09u, 0x00u, 0x03u, 0x12u, 0x02u, 0x85u, 0x60u, 0x0Du};
    uint8_t stream[sizeof(prefix) + sizeof(good)];
    ecsp_rx_t rx;
    ecsp_frame_t frame;

    (void)memcpy(stream, prefix, sizeof(prefix));
    (void)memcpy(&stream[sizeof(prefix)], good, sizeof(good));

    ecsp_rx_init(&rx);
    feed(&rx, stream, sizeof(stream), 0u);
    CHECK(ecsp_rx_poll(&rx, 0u, &frame));
    CHECK_EQ(frame.command, 0x12u);
}

TEST(ecsp_rx, accepts_completion_at_exactly_the_half_packet_deadline)
{
    static const uint8_t raw[] = {0xFAu, 0x09u, 0x00u, 0x03u, 0x12u, 0x02u, 0x85u, 0x60u, 0x0Du};
    ecsp_rx_t rx;
    ecsp_frame_t frame;

    ecsp_rx_init(&rx);
    feed(&rx, raw, sizeof(raw) - 1u, 0u);
    feed_byte(&rx, raw[sizeof(raw) - 1u], 50000u);
    CHECK(ecsp_rx_poll(&rx, 50000u, &frame));
    CHECK_EQ(frame.command, 0x12u);
}

TEST(ecsp_rx, rejects_a_frame_whose_last_byte_is_one_microsecond_late)
{
    static const uint8_t raw[] = {0xFAu, 0x09u, 0x00u, 0x03u, 0x12u, 0x02u, 0x85u, 0x60u, 0x0Du};
    ecsp_rx_t rx;
    ecsp_frame_t frame;

    ecsp_rx_init(&rx);
    feed(&rx, raw, sizeof(raw) - 1u, 0u);
    feed_byte(&rx, raw[sizeof(raw) - 1u], 50001u);
    CHECK(!ecsp_rx_poll(&rx, 50001u, &frame));
}

TEST(ecsp_rx, rescans_after_a_candidate_timeout_and_uses_original_timestamps)
{
    static const uint8_t prefix[] = {0xFAu, 0x40u, 0x00u};
    static const uint8_t second_head[] = {0xFAu, 0x09u, 0x00u};
    static const uint8_t second_tail[] = {0x03u, 0x12u, 0x02u, 0x85u, 0x60u, 0x0Du};
    ecsp_rx_t rx;
    ecsp_frame_t frame;

    ecsp_rx_init(&rx);
    feed(&rx, prefix, sizeof(prefix), 0u);
    feed(&rx, second_head, sizeof(second_head), 40000u);
    CHECK(!ecsp_rx_poll(&rx, 55000u, &frame));

    feed(&rx, second_tail, sizeof(second_tail), 90000u);
    CHECK(ecsp_rx_poll(&rx, 90000u, &frame));
    CHECK_EQ(frame.command, 0x12u);
}

TEST(ecsp_rx, rejects_the_rescanned_candidate_when_it_finishes_past_its_own_deadline)
{
    static const uint8_t prefix[] = {0xFAu, 0x40u, 0x00u};
    static const uint8_t second_head[] = {0xFAu, 0x09u, 0x00u};
    static const uint8_t second_tail[] = {0x03u, 0x12u, 0x02u, 0x85u, 0x60u, 0x0Du};
    ecsp_rx_t rx;
    ecsp_frame_t frame;

    ecsp_rx_init(&rx);
    feed(&rx, prefix, sizeof(prefix), 0u);
    feed(&rx, second_head, sizeof(second_head), 40000u);
    CHECK(!ecsp_rx_poll(&rx, 55000u, &frame));

    feed(&rx, second_tail, sizeof(second_tail), 91000u);
    CHECK(!ecsp_rx_poll(&rx, 91000u, &frame));
}

TEST(ecsp_rx, parses_a_later_frame_after_the_first_candidate_expires)
{
    static const uint8_t prefix[] = {0xFAu, 0x40u, 0x00u};
    static const uint8_t good[] = {0xFAu, 0x09u, 0x00u, 0x03u, 0x12u, 0x02u, 0x85u, 0x60u, 0x0Du};
    uint8_t stream[sizeof(prefix) + sizeof(good)];
    ecsp_rx_t rx;
    ecsp_frame_t frame;

    (void)memcpy(stream, prefix, sizeof(prefix));
    (void)memcpy(&stream[sizeof(prefix)], good, sizeof(good));

    ecsp_rx_init(&rx);
    feed(&rx, stream, sizeof(stream), 0u);
    CHECK(!ecsp_rx_poll(&rx, 20000u, &frame));
    CHECK(ecsp_rx_poll(&rx, 50000u, &frame));
    CHECK_EQ(frame.command, 0x12u);
}

TEST(ecsp_rx, emits_two_consecutive_frames_in_order)
{
    static const uint8_t first[] = {0xFAu, 0x09u, 0x00u, 0x03u, 0x12u, 0x02u, 0x85u, 0x60u, 0x0Du};
    static const uint8_t second[] = {0xFAu, 0x09u, 0x00u, 0x03u, 0x10u, 0x02u, 0x84u, 0x00u, 0x0Du};
    uint8_t stream[sizeof(first) + sizeof(second)];
    ecsp_rx_t rx;
    ecsp_frame_t frame;

    (void)memcpy(stream, first, sizeof(first));
    (void)memcpy(&stream[sizeof(first)], second, sizeof(second));

    ecsp_rx_init(&rx);
    feed(&rx, stream, sizeof(stream), 0u);
    CHECK(ecsp_rx_poll(&rx, 0u, &frame));
    CHECK_EQ(frame.command, 0x12u);
    CHECK(ecsp_rx_poll(&rx, 0u, &frame));
    CHECK_EQ(frame.command, 0x10u);
}

TEST(ecsp_rx, records_a_uart_gap_and_resynchronises)
{
    static const uint8_t partial[] = {0xFAu, 0x09u, 0x00u, 0x03u};
    static const uint8_t good[] = {0xFAu, 0x09u, 0x00u, 0x03u, 0x12u, 0x02u, 0x85u, 0x60u, 0x0Du};
    ecsp_rx_t rx;
    ecsp_frame_t frame;

    ecsp_rx_init(&rx);
    feed(&rx, partial, sizeof(partial), 0u);
    ecsp_rx_note_gap(&rx);
    feed(&rx, good, sizeof(good), 1000u);
    CHECK(ecsp_rx_poll(&rx, 1000u, &frame));
    CHECK_EQ(frame.command, 0x12u);
    CHECK(ecsp_rx_overflow_count(&rx) >= 1u);
}

TEST(ecsp_rx, window_overflow_drops_oldest_and_still_parses_the_tail_frame)
{
    static const uint8_t good[] = {0xFAu, 0x09u, 0x00u, 0x03u, 0x12u, 0x02u, 0x85u, 0x60u, 0x0Du};
    uint8_t filler[600];
    ecsp_rx_t rx;
    ecsp_frame_t frame;

    (void)memset(filler, 0x00, sizeof(filler));
    ecsp_rx_init(&rx);
    feed(&rx, filler, sizeof(filler), 0u);
    feed(&rx, good, sizeof(good), 1u);
    CHECK(ecsp_rx_poll(&rx, 1u, &frame));
    CHECK_EQ(frame.command, 0x12u);
    CHECK(ecsp_rx_overflow_count(&rx) > 0u);
}
