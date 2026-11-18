#include "test_support.h"

#include <string.h>

#include "app/ecsp/ecsp_codes.h"
#include "app/ecsp/ecsp_frame.h"
#include "app/psg_reports.h"
#include "psg_fixture.h"

#define HEARTBEAT_FRAME_US 781u
#define STATUS_REPORT_FRAME_US 1909u

static bool send_command(psg_fixture_t *fixture, uint8_t command, const uint8_t *payload, uint16_t payload_length,
                         uint8_t *response, size_t capacity, size_t *response_length)
{
    uint8_t request[PSG_ECSP_MAX_FRAME_BYTES];
    size_t length = ecsp_frame_encode(request, sizeof(request), 0x03u, 0x02u, command, payload, payload_length);

    if (length == 0u) {
        return false;
    }
    return fixture_exchange(fixture, request, length, response, capacity, response_length);
}

static uint8_t send_code(psg_fixture_t *fixture, uint8_t command, const uint8_t *payload, uint16_t payload_length)
{
    uint8_t response[64];
    size_t response_length = 0u;

    if (!send_command(fixture, command, payload, payload_length, response, sizeof(response), &response_length)) {
        return 0xFFu;
    }
    if (response_length <= 6u) {
        return 0xFEu;
    }
    return response[6];
}

static void set_upload(psg_fixture_t *fixture, bool enabled)
{
    uint8_t payload[1];

    payload[0] = enabled ? 1u : 0u;
    CHECK_EQ(send_code(fixture, ECSP_CMD_SET_UPLOAD_MODE, payload, 1u), ECSP_RSP_SUCCESS);
}

static bool take_parsed(psg_fixture_t *fixture, ecsp_frame_t *frame)
{
    uint8_t raw[PSG_ECSP_MAX_FRAME_BYTES];
    size_t length = 0u;

    if (!fixture_take_tx(fixture, raw, sizeof(raw), &length)) {
        return false;
    }
    return ecsp_frame_parse(raw, length, frame);
}

TEST(upload, default_off_suppresses_the_completion_report)
{
    psg_fixture_t fixture;
    uint8_t raw[64];
    size_t length = 0u;

    fixture_ready(&fixture);
    CHECK(fixture_configure_run(&fixture));
    CHECK(fixture_start_run(&fixture));
    fixture_complete_run(&fixture, PSG_RUN_END_NORMAL, PSG_CHANNEL_STOPPED, 7.0f, PSG_CHANNEL_STOPPED, 8.0f, 1u);
    fixture_pump(&fixture);

    CHECK(!fixture_take_tx(&fixture, raw, sizeof(raw), &length));
}

TEST(upload, enabled_mode_reports_one_status_block_per_completion)
{
    psg_fixture_t fixture;
    ecsp_frame_t frame;

    fixture_ready(&fixture);
    CHECK(fixture_configure_run(&fixture));
    set_upload(&fixture, true);

    CHECK(fixture_start_run(&fixture));
    fixture_complete_run(&fixture, PSG_RUN_END_NORMAL, PSG_CHANNEL_STOPPED, 7.0f, PSG_CHANNEL_STOPPED, 8.0f, 1u);
    fixture_pump(&fixture);
    CHECK(take_parsed(&fixture, &frame));
    CHECK_EQ(frame.command, 0x3Cu);
    CHECK_EQ(frame.payload_length, PSG_STATUS_BLOCK_BYTES);
    CHECK_EQ(frame.payload[0], 0x00u); /* direct trigger */
    CHECK_EQ(frame.payload[1], PSG_CHANNEL_STOPPED);
    CHECK_EQ(frame.payload[2], PSG_CHANNEL_STOPPED);
    CHECK_NEAR(ecsp_read_f32(&frame.payload[3]), 7.0f, 0.001f);
    CHECK_NEAR(ecsp_read_f32(&frame.payload[7]), 8.0f, 0.001f);
    CHECK_EQ(ecsp_read_u16(&frame.payload[11]), 1u);

    fixture_advance(&fixture, STATUS_REPORT_FRAME_US);
    fixture_pump(&fixture);

    CHECK(fixture_start_run(&fixture));
    fixture_complete_run(&fixture, PSG_RUN_END_NORMAL, PSG_CHANNEL_STOPPED, 9.0f, PSG_CHANNEL_STOPPED, 10.0f, 1u);
    fixture_pump(&fixture);
    CHECK(take_parsed(&fixture, &frame));
    CHECK_EQ(frame.command, 0x3Cu);
    CHECK_NEAR(ecsp_read_f32(&frame.payload[3]), 9.0f, 0.001f);
    CHECK_NEAR(ecsp_read_f32(&frame.payload[7]), 10.0f, 0.001f);
}

TEST(upload, status_report_does_not_consume_the_snapshot)
{
    psg_fixture_t fixture;
    ecsp_frame_t frame;
    const psg_run_snapshot_t *snapshot;
    uint8_t response[64];
    size_t response_length = 0u;

    fixture_ready(&fixture);
    CHECK(fixture_configure_run(&fixture));
    set_upload(&fixture, true);
    CHECK(fixture_start_run(&fixture));
    fixture_complete_run(&fixture, PSG_RUN_END_NORMAL, PSG_CHANNEL_STOPPED, 7.0f, PSG_CHANNEL_STOPPED, 8.0f, 1u);
    fixture_pump(&fixture);
    CHECK(take_parsed(&fixture, &frame));
    fixture_advance(&fixture, STATUS_REPORT_FRAME_US);
    fixture_pump(&fixture);

    snapshot = psg_run_snapshot(&fixture.run, 0u);
    CHECK(snapshot != NULL);
    CHECK(snapshot->valid);

    CHECK(send_command(&fixture, 0x31u, NULL, 0u, response, sizeof(response), &response_length));
    CHECK_EQ(response[6], ECSP_RSP_SUCCESS);
    CHECK_NEAR(ecsp_read_f32(&response[10]), 7.0f, 0.001f);
    CHECK_EQ(ecsp_read_u16(&response[18]), 1u);
}

TEST(upload, current_anomaly_termination_reports_the_channel_evidence)
{
    psg_fixture_t fixture;
    ecsp_frame_t frame;
    uint8_t raw[64];
    size_t length = 0u;

    fixture_ready(&fixture);
    CHECK(fixture_configure_run(&fixture));
    set_upload(&fixture, true);
    CHECK(fixture_start_run(&fixture));

    fixture_complete_run(&fixture, PSG_RUN_END_OVERCURRENT, PSG_CHANNEL_OVERCURRENT, 12.0f,
                         PSG_CHANNEL_STOPPED, 0.0f, 0u);
    fixture_pump(&fixture);
    CHECK(take_parsed(&fixture, &frame));
    CHECK_EQ(frame.command, 0x3Cu);
    CHECK_EQ(frame.payload_length, PSG_STATUS_BLOCK_BYTES);
    CHECK_EQ(frame.payload[1], PSG_CHANNEL_OVERCURRENT);
    CHECK_EQ(frame.payload[2], PSG_CHANNEL_STOPPED);
    CHECK_NEAR(ecsp_read_f32(&frame.payload[3]), 12.0f, 0.001f);
    CHECK_NEAR(ecsp_read_f32(&frame.payload[7]), 0.0f, 0.001f);
    CHECK_EQ(ecsp_read_u16(&frame.payload[11]), 0u);
    CHECK_EQ(psg_run_state(&fixture.run), PSG_RUN_CURRENT_EXCEPTION);

    fixture_advance(&fixture, STATUS_REPORT_FRAME_US);
    fixture_pump(&fixture);
    CHECK(!fixture_take_tx(&fixture, raw, sizeof(raw), &length));
}

TEST(upload, failed_clear_replies_then_emits_one_fault_report_when_upload_is_on)
{
    psg_fixture_t fixture;
    uint8_t request[16];
    uint8_t response[64];
    ecsp_frame_t frame;
    size_t response_length = 0u;
    size_t length;

    fixture_ready(&fixture);
    CHECK(fixture_configure_run(&fixture));
    set_upload(&fixture, true);
    CHECK(fixture_start_run(&fixture));
    fixture_complete_run(&fixture, PSG_RUN_END_OVERCURRENT, PSG_CHANNEL_OVERCURRENT, 12.0f,
                         PSG_CHANNEL_STOPPED, 0.0f, 0u);
    fixture_pump(&fixture);
    CHECK(take_parsed(&fixture, &frame)); /* 0x3C from the termination */

    fixture.board.latch = true;
    fixture.board.latch_clears_on_reset = false;
    length = ecsp_frame_encode(request, sizeof(request), 0x03u, 0x02u, 0x3Du, NULL, 0u);
    fixture_feed_frame(&fixture, request, length);
    fixture_pump(&fixture);
    CHECK(!fixture_take_tx(&fixture, response, sizeof(response), &response_length));

    fixture_advance(&fixture, 5000u);
    fixture_pump(&fixture);
    CHECK(fixture_take_tx(&fixture, response, sizeof(response), &response_length));
    CHECK_EQ(response[6], 0x20u);

    fixture_advance(&fixture, psg_uart_frame_duration_us((uint32_t)response_length));
    fixture_pump(&fixture);
    CHECK(take_parsed(&fixture, &frame));
    CHECK_EQ(frame.command, ECSP_CMD_FAULT_REPORT);
    CHECK_EQ(frame.payload[0], PSG_FAULT_OVERCURRENT_LATCH);
}

TEST(upload, duplicate_handshake_restores_upload_default_off)
{
    psg_fixture_t fixture;
    uint8_t request[16];
    uint8_t response[64];
    size_t response_length = 0u;
    size_t length;

    fixture_ready(&fixture);
    CHECK(fixture_configure_run(&fixture));
    set_upload(&fixture, true);

    length = ecsp_frame_encode(request, sizeof(request), 0x03u, 0x02u, ECSP_CMD_HANDSHAKE, NULL, 0u);
    CHECK(fixture_exchange(&fixture, request, length, response, sizeof(response), &response_length));
    CHECK_EQ(response[6], ECSP_RSP_SUCCESS);

    psg_run_complete(&fixture.run, PSG_RUN_END_NORMAL,
                     (const psg_channel_result_t[]){{PSG_CHANNEL_STOPPED, 1.0f}, {PSG_CHANNEL_STOPPED, 1.0f}}, 1u);
    fixture_pump(&fixture);
    CHECK(!fixture_take_tx(&fixture, response, sizeof(response), &response_length));
}

TEST(upload, fault_session_exception_reports_even_when_upload_is_off)
{
    psg_fixture_t fixture;
    ecsp_frame_t frame;

    fixture_ready(&fixture);
    CHECK_EQ(send_code(&fixture, ECSP_CMD_SET_UPLOAD_MODE, (const uint8_t[]){0x00u}, 1u), ECSP_RSP_SUCCESS);

    psg_fault_set(&fixture.fault, PSG_FAULT_OVERCURRENT_LATCH);
    fixture_pump(&fixture);
    CHECK(take_parsed(&fixture, &frame));
    CHECK_EQ(frame.command, ECSP_CMD_FAULT_REPORT);
    CHECK_EQ(frame.payload_length, 1u);
    CHECK_EQ(frame.payload[0], PSG_FAULT_OVERCURRENT_LATCH);
}

TEST(upload, a_new_fault_bit_reports_once_and_not_again)
{
    psg_fixture_t fixture;
    ecsp_frame_t frame;
    uint8_t raw[64];
    size_t length = 0u;

    fixture_ready(&fixture);
    psg_fault_set(&fixture.fault, PSG_FAULT_OVERCURRENT_LATCH);
    fixture_pump(&fixture);
    CHECK(take_parsed(&fixture, &frame));

    fixture_advance(&fixture, 868u);
    fixture_pump(&fixture);
    CHECK(!fixture_take_tx(&fixture, raw, sizeof(raw), &length));

    psg_fault_set(&fixture.fault, PSG_FAULT_OVERCURRENT_LATCH);
    fixture_pump(&fixture);
    CHECK(!fixture_take_tx(&fixture, raw, sizeof(raw), &length));
}

TEST(upload, fault_report_is_suppressed_before_handshake)
{
    psg_fixture_t fixture;
    uint8_t raw[64];
    size_t length = 0u;

    fixture_init(&fixture);
    fixture_pump(&fixture);
    CHECK(fixture_take_tx(&fixture, raw, sizeof(raw), &length));
    fixture_advance(&fixture, 781u);
    fixture_pump(&fixture);

    psg_fault_set(&fixture.fault, PSG_FAULT_OVERCURRENT_LATCH);
    fixture_pump(&fixture);
    CHECK(!fixture_take_tx(&fixture, raw, sizeof(raw), &length));
}

TEST(upload, reply_jumps_ahead_of_a_queued_upload)
{
    psg_fixture_t fixture;
    ecsp_frame_t frame;
    static const uint8_t get_fault[] = {0xFAu, 0x09u, 0x00u, 0x03u, 0x12u, 0x02u, 0x85u, 0x60u, 0x0Du};

    fixture_ready(&fixture);
    CHECK(fixture_configure_run(&fixture));
    set_upload(&fixture, true);

    CHECK(fixture_start_run(&fixture));
    fixture_complete_run(&fixture, PSG_RUN_END_NORMAL, PSG_CHANNEL_STOPPED, 1.0f, PSG_CHANNEL_STOPPED, 1.0f, 1u);
    fixture_pump(&fixture);
    CHECK(take_parsed(&fixture, &frame));
    CHECK_EQ(frame.command, 0x3Cu);

    /* A second completion queues another upload while the first is on the
     * wire, then a reply arrives. */
    psg_run_complete(&fixture.run, PSG_RUN_END_NORMAL,
                     (const psg_channel_result_t[]){{PSG_CHANNEL_STOPPED, 2.0f}, {PSG_CHANNEL_STOPPED, 2.0f}}, 1u);
    fixture_pump(&fixture);
    fixture_feed_frame(&fixture, get_fault, sizeof(get_fault));
    fixture_pump(&fixture);

    fixture_advance(&fixture, STATUS_REPORT_FRAME_US);
    fixture_pump(&fixture);
    CHECK(take_parsed(&fixture, &frame));
    CHECK_EQ(frame.command, ECSP_CMD_GET_FAULT);

    fixture_advance(&fixture, 954u);
    fixture_pump(&fixture);
    CHECK(take_parsed(&fixture, &frame));
    CHECK_EQ(frame.command, 0x3Cu);
}

TEST(upload, heartbeat_continues_through_a_direct_trigger_run)
{
    psg_fixture_t fixture;
    ecsp_frame_t frame;
    uint64_t base;
    uint8_t index;

    fixture_ready(&fixture);
    CHECK(fixture_configure_run(&fixture));
    CHECK(fixture_start_run(&fixture));
    base = fixture.clock.now_us;

    for (index = 1u; index <= 3u; index++) {
        fixture_advance(&fixture, 1000000u - HEARTBEAT_FRAME_US);
        fixture_pump(&fixture);
        CHECK(take_parsed(&fixture, &frame));
        CHECK_EQ(frame.command, ECSP_CMD_HEARTBEAT);
        CHECK_EQ(fixture.uart.tx_start_us,
                 base + ((uint64_t)index * 1000000u) - HEARTBEAT_FRAME_US);
        CHECK_EQ(psg_run_state(&fixture.run), PSG_RUN_RUNNING);
        fixture_advance(&fixture, HEARTBEAT_FRAME_US);
        fixture_pump(&fixture);
    }
}

TEST(upload, heartbeat_continues_through_an_r_wave_long_run)
{
    psg_fixture_t fixture;
    ecsp_frame_t frame;
    uint8_t r_wave[6];
    uint64_t base;
    uint8_t index;

    fixture_ready(&fixture);
    CHECK_EQ(send_code(&fixture, 0x32u, (const uint8_t[]){0x01u}, 1u), ECSP_RSP_SUCCESS);
    ecsp_write_u16(&r_wave[0], 0u);
    ecsp_write_u16(&r_wave[2], 200u);
    ecsp_write_u16(&r_wave[4], 2u);
    CHECK_EQ(send_code(&fixture, 0x36u, r_wave, sizeof(r_wave)), ECSP_RSP_SUCCESS);
    CHECK(fixture_configure_run(&fixture));
    CHECK(fixture_start_run(&fixture));
    CHECK_EQ(psg_run_state(&fixture.run), PSG_RUN_RUNNING);
    base = fixture.clock.now_us;

    for (index = 1u; index <= 2u; index++) {
        fixture_advance(&fixture, 1000000u - HEARTBEAT_FRAME_US);
        fixture_pump(&fixture);
        CHECK(take_parsed(&fixture, &frame));
        CHECK_EQ(frame.command, ECSP_CMD_HEARTBEAT);
        CHECK_EQ(fixture.uart.tx_start_us,
                 base + ((uint64_t)index * 1000000u) - HEARTBEAT_FRAME_US);
        CHECK_EQ(psg_run_state(&fixture.run), PSG_RUN_RUNNING);
        fixture_advance(&fixture, HEARTBEAT_FRAME_US);
        fixture_pump(&fixture);
    }
}
