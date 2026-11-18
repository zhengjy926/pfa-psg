#include "test_support.h"

#include <string.h>

#include "app/ecsp/ecsp_frame.h"

TEST(frame, crc_self_check_matches_prm_021)
{
    static const uint8_t check[] = "123456789";

    CHECK_EQ(ecsp_crc16(check, 9u), 0x4B37u);
}

TEST(frame, encodes_documented_start_request)
{
    static const uint8_t payload[] = {0x1Bu, 0x01u};
    static const uint8_t expected[] = {0xFAu, 0x0Bu, 0x00u, 0x03u, 0x30u, 0x02u,
                                       0x1Bu, 0x01u, 0x80u, 0x30u, 0x0Du};
    uint8_t out[32];
    size_t length = ecsp_frame_encode(out, sizeof(out), 0x03u, 0x02u, 0x30u, payload, sizeof(payload));

    CHECK_EQ(length, sizeof(expected));
    CHECK(memcmp(out, expected, sizeof(expected)) == 0);
}

TEST(frame, encodes_documented_heartbeat)
{
    static const uint8_t expected[] = {0xFAu, 0x09u, 0x00u, 0x03u, 0x10u,
                                       0x02u, 0x84u, 0x00u, 0x0Du};
    uint8_t out[16];
    size_t length = ecsp_frame_encode(out, sizeof(out), 0x03u, 0x02u, 0x10u, NULL, 0u);

    CHECK_EQ(length, sizeof(expected));
    CHECK(memcmp(out, expected, sizeof(expected)) == 0);
}

TEST(frame, parses_documented_get_fault_request)
{
    static const uint8_t raw[] = {0xFAu, 0x09u, 0x00u, 0x03u, 0x12u, 0x02u, 0x85u, 0x60u, 0x0Du};
    ecsp_frame_t frame;

    CHECK(ecsp_frame_parse(raw, sizeof(raw), &frame));
    CHECK_EQ(frame.product, 0x03u);
    CHECK_EQ(frame.board, 0x02u);
    CHECK_EQ(frame.command, 0x12u);
    CHECK_EQ(frame.payload_length, 0u);
}

TEST(frame, rejects_corrupted_crc)
{
    static const uint8_t raw[] = {0xFAu, 0x0Au, 0x00u, 0x03u, 0x32u, 0x02u, 0x01u, 0x61u, 0x9Au, 0x0Du};
    uint8_t corrupt[sizeof(raw)];
    ecsp_frame_t frame;

    (void)memcpy(corrupt, raw, sizeof(raw));
    corrupt[6] ^= 0x01u;
    CHECK(!ecsp_frame_parse(corrupt, sizeof(corrupt), &frame));
}

TEST(frame, rejects_wrong_tail_and_length)
{
    static const uint8_t raw[] = {0xFAu, 0x09u, 0x00u, 0x03u, 0x12u, 0x02u, 0x85u, 0x60u, 0x0Du};
    uint8_t bad_tail[sizeof(raw)];
    uint8_t bad_length[sizeof(raw)];
    ecsp_frame_t frame;

    (void)memcpy(bad_tail, raw, sizeof(raw));
    bad_tail[sizeof(raw) - 1u] = 0x00u;
    CHECK(!ecsp_frame_parse(bad_tail, sizeof(bad_tail), &frame));

    (void)memcpy(bad_length, raw, sizeof(raw));
    bad_length[1] = 0x0Au;
    CHECK(!ecsp_frame_parse(bad_length, sizeof(bad_length), &frame));
}

TEST(frame, honours_minimum_and_maximum_frame_length)
{
    static const uint8_t raw[] = {0xFAu, 0x09u, 0x00u, 0x03u, 0x01u, 0x02u, 0x88u, 0x50u, 0x0Du};
    uint8_t payload[PSG_ECSP_MAX_PAYLOAD_BYTES];
    uint8_t out[PSG_ECSP_MAX_FRAME_BYTES];
    ecsp_frame_t frame;
    size_t length;

    CHECK(ecsp_frame_parse(raw, sizeof(raw), &frame));

    (void)memset(payload, 0x5Au, sizeof(payload));
    length = ecsp_frame_encode(out, sizeof(out), 0x03u, 0x02u, 0x35u, payload, sizeof(payload));
    CHECK_EQ(length, PSG_ECSP_MAX_FRAME_BYTES);
    CHECK(ecsp_frame_parse(out, length, &frame));
    CHECK_EQ(frame.payload_length, PSG_ECSP_MAX_PAYLOAD_BYTES);

    CHECK_EQ(ecsp_frame_encode(out, sizeof(out), 0x03u, 0x02u, 0x35u, payload,
                               (uint16_t)(PSG_ECSP_MAX_PAYLOAD_BYTES + 1u)),
             0u);
}

TEST(frame, payload_codecs_round_trip)
{
    uint8_t raw[12];

    ecsp_write_u16(&raw[0], 0x1234u);
    ecsp_write_u32(&raw[2], 0x89ABCDEFu);
    ecsp_write_f32(&raw[6], 17.2f);

    CHECK_EQ(ecsp_read_u16(&raw[0]), 0x1234u);
    CHECK_EQ(ecsp_read_u32(&raw[2]), 0x89ABCDEFu);
    CHECK(ecsp_read_f32(&raw[6]) > 17.19f);
    CHECK(ecsp_read_f32(&raw[6]) < 17.21f);
}
