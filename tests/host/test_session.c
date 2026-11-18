#include "test_support.h"

#include <string.h>

#include "app/ecsp/ecsp_codes.h"
#include "app/ecsp/ecsp_frame.h"
#include "psg_fixture.h"

#define HEARTBEAT_FRAME_US 781u
#define BOOT_FRAME_US 781u

static uint8_t reply_code_of(const uint8_t *response, size_t length)
{
    CHECK(length > 6u);
    return response[6];
}

TEST(session, sends_boot_handshake_report_within_prm_116)
{
    psg_fixture_t fixture;
    uint8_t frame[32];
    size_t length = 0u;
    ecsp_frame_t parsed;

    fixture_init(&fixture);
    fixture_pump(&fixture);
    CHECK(fixture_take_tx(&fixture, frame, sizeof(frame), &length));
    CHECK_EQ(fixture.clock.now_us <= 500000u, 1);
    CHECK(ecsp_frame_parse(frame, length, &parsed));
    CHECK_EQ(parsed.command, ECSP_CMD_HANDSHAKE);
    CHECK_EQ(parsed.payload_length, 0u);
}

TEST(session, repeats_the_boot_report_at_one_hertz_until_handshake)
{
    psg_fixture_t fixture;
    uint8_t frame[32];
    size_t length = 0u;
    uint64_t start;

    fixture_init(&fixture);
    fixture_pump(&fixture);
    CHECK(fixture_take_tx(&fixture, frame, sizeof(frame), &length));
    fixture_advance(&fixture, BOOT_FRAME_US);
    fixture_pump(&fixture);

    start = fixture.clock.now_us;
    fixture_advance(&fixture, 1000000u - BOOT_FRAME_US);
    fixture_pump(&fixture);
    CHECK(fixture_take_tx(&fixture, frame, sizeof(frame), &length));
    CHECK_EQ(fixture.uart.tx_start_us, 1000000u);
    CHECK_EQ(start <= fixture.uart.tx_start_us, 1);
}

TEST(session, replies_to_handshake_and_stops_boot_reports)
{
    psg_fixture_t fixture;
    uint8_t response[32];
    size_t response_length = 0u;
    static const uint8_t request[] = {0xFAu, 0x09u, 0x00u, 0x03u, 0x01u, 0x02u, 0x88u, 0x50u, 0x0Du};

    fixture_init(&fixture);
    fixture_pump(&fixture);
    (void)fixture_take_tx(&fixture, response, sizeof(response), &response_length);
    fixture_advance(&fixture, BOOT_FRAME_US);
    fixture_pump(&fixture);

    CHECK(fixture_exchange(&fixture, request, sizeof(request), response, sizeof(response), &response_length));
    CHECK_EQ(response_length, 10u);
    CHECK_EQ(reply_code_of(response, response_length), ECSP_RSP_SUCCESS);

    /* No further periodic handshake reports after the reply completed. */
    fixture_advance(&fixture, 100000u);
    fixture_pump(&fixture);
    CHECK(!fixture_take_tx(&fixture, response, sizeof(response), &response_length));
}

TEST(session, rejects_supported_request_before_handshake_with_0x10)
{
    psg_fixture_t fixture;
    static const uint8_t request[] = {0xFAu, 0x09u, 0x00u, 0x03u, 0x12u, 0x02u, 0x85u, 0x60u, 0x0Du};
    uint8_t response[32];
    size_t response_length = 0u;

    fixture_init(&fixture);
    fixture_pump(&fixture);
    (void)fixture_take_tx(&fixture, response, sizeof(response), &response_length);
    fixture_advance(&fixture, BOOT_FRAME_US);
    fixture_pump(&fixture);

    CHECK(fixture_exchange(&fixture, request, sizeof(request), response, sizeof(response), &response_length));
    CHECK_EQ(reply_code_of(response, response_length), ECSP_RSP_NOT_HANDSHAKEN);
}

TEST(session, rejects_unknown_request_before_handshake_with_0x04)
{
    psg_fixture_t fixture;
    uint8_t request[16];
    uint8_t response[32];
    size_t response_length = 0u;
    size_t length = ecsp_frame_encode(request, sizeof(request), 0x03u, 0x02u, 0x08u, NULL, 0u);

    fixture_init(&fixture);
    fixture_pump(&fixture);
    (void)fixture_take_tx(&fixture, response, sizeof(response), &response_length);
    fixture_advance(&fixture, BOOT_FRAME_US);
    fixture_pump(&fixture);

    CHECK(fixture_exchange(&fixture, request, length, response, sizeof(response), &response_length));
    CHECK_EQ(reply_code_of(response, response_length), ECSP_RSP_UNSUPPORTED);
}

TEST(session, reads_software_and_hardware_versions_and_serial)
{
    psg_fixture_t fixture;
    uint8_t request[16];
    uint8_t response[64];
    size_t response_length = 0u;
    size_t length;
    ecsp_frame_t parsed;

    fixture_ready(&fixture);

    length = ecsp_frame_encode(request, sizeof(request), 0x03u, 0x02u, ECSP_CMD_GET_SW_VERSION, NULL, 0u);
    CHECK(fixture_exchange(&fixture, request, length, response, sizeof(response), &response_length));
    CHECK(ecsp_frame_parse(response, response_length, &parsed));
    CHECK_EQ(parsed.payload_length, 8u);
    CHECK_EQ(parsed.payload[0], ECSP_RSP_SUCCESS);
    CHECK(memcmp(&parsed.payload[1], "0.0.0.0", 7u) == 0);

    length = ecsp_frame_encode(request, sizeof(request), 0x03u, 0x02u, ECSP_CMD_GET_HW_VERSION, NULL, 0u);
    CHECK(fixture_exchange(&fixture, request, length, response, sizeof(response), &response_length));
    CHECK(ecsp_frame_parse(response, response_length, &parsed));
    CHECK_EQ(parsed.payload_length, 8u);
    CHECK(memcmp(&parsed.payload[1], "0.0.0.0", 7u) == 0);

    length = ecsp_frame_encode(request, sizeof(request), 0x03u, 0x02u, ECSP_CMD_GET_SERIAL, NULL, 0u);
    CHECK(fixture_exchange(&fixture, request, length, response, sizeof(response), &response_length));
    CHECK(ecsp_frame_parse(response, response_length, &parsed));
    CHECK_EQ(parsed.payload_length, 13u);
    CHECK(memcmp(&parsed.payload[1], "SN0000000000", 12u) == 0);
}

TEST(session, rejects_a_handshake_with_payload_using_0x02)
{
    psg_fixture_t fixture;
    uint8_t request[16];
    uint8_t response[32];
    size_t response_length = 0u;
    size_t length = ecsp_frame_encode(request, sizeof(request), 0x03u, 0x02u, ECSP_CMD_HANDSHAKE,
                                      (const uint8_t[]){0x00u}, 1u);

    fixture_ready(&fixture);
    CHECK(fixture_exchange(&fixture, request, length, response, sizeof(response), &response_length));
    CHECK_EQ(reply_code_of(response, response_length), ECSP_RSP_BAD_LENGTH);
}

TEST(session, stays_connected_regardless_of_host_version_check)
{
    psg_fixture_t fixture;
    uint8_t request[16];
    uint8_t response[64];
    size_t response_length = 0u;
    size_t length;

    fixture_ready(&fixture);
    length = ecsp_frame_encode(request, sizeof(request), 0x03u, 0x02u, ECSP_CMD_GET_FAULT, NULL, 0u);
    CHECK(fixture_exchange(&fixture, request, length, response, sizeof(response), &response_length));
    CHECK_EQ(reply_code_of(response, response_length), ECSP_RSP_SUCCESS);
}

TEST(session, heartbeat_completion_interval_is_one_second_while_idle)
{
    psg_fixture_t fixture;
    uint8_t frame[32];
    size_t length = 0u;
    uint64_t base;

    fixture_ready(&fixture);
    base = fixture.clock.now_us;

    fixture_advance(&fixture, 1000000u - HEARTBEAT_FRAME_US - 1u);
    fixture_pump(&fixture);
    CHECK(!fixture_take_tx(&fixture, frame, sizeof(frame), &length));

    fixture_advance(&fixture, 1u);
    fixture_pump(&fixture);
    CHECK(fixture_take_tx(&fixture, frame, sizeof(frame), &length));
    CHECK_EQ(fixture.uart.tx_start_us, base + 1000000u - HEARTBEAT_FRAME_US);
    fixture_advance(&fixture, HEARTBEAT_FRAME_US);
    fixture_pump(&fixture);

    fixture_advance(&fixture, 1000000u - HEARTBEAT_FRAME_US);
    fixture_pump(&fixture);
    CHECK(fixture_take_tx(&fixture, frame, sizeof(frame), &length));
    CHECK_EQ(fixture.uart.tx_start_us, base + 2000000u - HEARTBEAT_FRAME_US);
}

TEST(session, heartbeat_rebases_on_the_completion_of_an_intervening_reply)
{
    psg_fixture_t fixture;
    uint8_t request[16];
    uint8_t response[64];
    uint8_t frame[32];
    size_t response_length = 0u;
    size_t length;
    uint64_t reply_completion;
    fixture_ready(&fixture);
    length = ecsp_frame_encode(request, sizeof(request), 0x03u, 0x02u, ECSP_CMD_GET_FAULT, NULL, 0u);
    CHECK(fixture_exchange(&fixture, request, length, response, sizeof(response), &response_length));

    reply_completion = fixture.clock.now_us;

    fixture_advance(&fixture, 1000000u - HEARTBEAT_FRAME_US);
    fixture_pump(&fixture);
    CHECK(fixture_take_tx(&fixture, frame, sizeof(frame), &length));
    CHECK_EQ(fixture.uart.tx_start_us, reply_completion + 1000000u - HEARTBEAT_FRAME_US);
}

TEST(session, duplicate_handshake_terminates_run_and_invalidates_config)
{
    psg_fixture_t fixture;
    uint8_t request[16];
    uint8_t response[64];
    size_t response_length = 0u;
    size_t length;

    fixture_ready(&fixture);
    CHECK(fixture_configure_run(&fixture));
    CHECK(fixture_start_run(&fixture));
    CHECK_EQ(psg_run_state(&fixture.run), PSG_RUN_RUNNING);

    length = ecsp_frame_encode(request, sizeof(request), 0x03u, 0x02u, ECSP_CMD_HANDSHAKE, NULL, 0u);
    CHECK(fixture_exchange(&fixture, request, length, response, sizeof(response), &response_length));
    CHECK_EQ(reply_code_of(response, response_length), ECSP_RSP_SUCCESS);
    CHECK_EQ(psg_run_state(&fixture.run), PSG_RUN_IDLE);

    length = ecsp_frame_encode(request, sizeof(request), 0x03u, 0x02u, 0x3Bu, NULL, 0u);
    CHECK(fixture_exchange(&fixture, request, length, response, sizeof(response), &response_length));
    CHECK_EQ(reply_code_of(response, response_length), ECSP_RSP_SUCCESS);
    CHECK_EQ(response[7], 0x00u); /* PRM-327 unconfigured sentinel */
    CHECK_EQ(response[8], 0x00u);
    CHECK_EQ(response[9], 0x00u);
    CHECK_EQ(response[10], 0x00u);

    length = ecsp_frame_encode(request, sizeof(request), 0x03u, 0x02u, 0x30u,
                               (const uint8_t[]){0x30u, 0x01u}, 2u);
    CHECK(fixture_exchange(&fixture, request, length, response, sizeof(response), &response_length));
    CHECK_EQ(reply_code_of(response, response_length), ECSP_RSP_PRECONDITION);
}

TEST(session, duplicate_handshake_clears_dedup_cache)
{
    psg_fixture_t fixture;
    uint8_t request[16];
    uint8_t response[64];
    size_t response_length = 0u;
    size_t length;

    fixture_ready(&fixture);
    CHECK(fixture_configure_run(&fixture));

    length = ecsp_frame_encode(request, sizeof(request), 0x03u, 0x02u, 0x30u,
                               (const uint8_t[]){0x77u, 0x01u}, 2u);
    CHECK(fixture_exchange(&fixture, request, length, response, sizeof(response), &response_length));
    CHECK_EQ(reply_code_of(response, response_length), ECSP_RSP_SUCCESS);

    /* Duplicate handshake resets the run and clears the transaction cache. */
    length = ecsp_frame_encode(request, sizeof(request), 0x03u, 0x02u, ECSP_CMD_HANDSHAKE, NULL, 0u);
    CHECK(fixture_exchange(&fixture, request, length, response, sizeof(response), &response_length));

    /* Replaying the old sequence number is now a new transaction and fails the
     * start gate because the sequence was invalidated. */
    length = ecsp_frame_encode(request, sizeof(request), 0x03u, 0x02u, 0x30u,
                               (const uint8_t[]){0x77u, 0x01u}, 2u);
    CHECK(fixture_exchange(&fixture, request, length, response, sizeof(response), &response_length));
    CHECK_EQ(reply_code_of(response, response_length), ECSP_RSP_PRECONDITION);
}
