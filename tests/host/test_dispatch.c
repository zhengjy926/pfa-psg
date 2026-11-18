#include "test_support.h"

#include <string.h>

#include "app/ecsp/ecsp_codes.h"
#include "app/ecsp/ecsp_frame.h"
#include "psg_fixture.h"
#include "run/run_config.h"

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

static uint8_t start_code(psg_fixture_t *fixture, uint8_t sequence, uint8_t control)
{
    uint8_t payload[2];

    payload[0] = sequence;
    payload[1] = control;
    return send_code(fixture, 0x30u, payload, 2u);
}

/* A newly set fault bit raises 0x11; consume it so the next exchange observes
 * the command reply rather than the asynchronous report. */
static void drain_fault_report(psg_fixture_t *fixture)
{
    uint8_t raw[64];
    size_t length = 0u;

    fixture_pump(fixture);
    if (fixture_take_tx(fixture, raw, sizeof(raw), &length)) {
        fixture_advance(fixture, psg_uart_frame_duration_us((uint32_t)length));
        fixture_pump(fixture);
    }
}

static void make_sequence_block(uint8_t block[22], uint8_t group_count, uint8_t index, uint16_t group_interval,
                                uint16_t bursts, uint16_t waves, uint16_t burst_interval, uint16_t pos_ns,
                                uint16_t neg_ns, uint32_t gap_ns, uint32_t interval_ns)
{
    (void)memset(block, 0, PSG_GROUP_ENCODED_BYTES);
    block[0] = group_count;
    block[1] = index;
    ecsp_write_u16(&block[2], group_interval);
    ecsp_write_u16(&block[4], bursts);
    ecsp_write_u16(&block[6], waves);
    ecsp_write_u16(&block[8], burst_interval);
    ecsp_write_u16(&block[10], pos_ns);
    ecsp_write_u16(&block[12], neg_ns);
    ecsp_write_u32(&block[14], gap_ns);
    ecsp_write_u32(&block[18], interval_ns);
}

TEST(dispatch, rejects_wrong_payload_length_after_handshake_with_0x02)
{
    psg_fixture_t fixture;

    fixture_ready(&fixture);
    CHECK_EQ(send_code(&fixture, 0x32u, NULL, 0u), ECSP_RSP_BAD_LENGTH);
}

TEST(dispatch, rejects_bad_parameter_after_length_with_0x03)
{
    psg_fixture_t fixture;
    static const uint8_t bad_mode[] = {0x02u};

    fixture_ready(&fixture);
    CHECK_EQ(send_code(&fixture, 0x32u, bad_mode, 1u), ECSP_RSP_BAD_PARAMETER);
}

TEST(dispatch, start_gate_returns_0x13_until_configuration_is_complete)
{
    psg_fixture_t fixture;

    fixture_ready(&fixture);
    CHECK_EQ(start_code(&fixture, 0x01u, 0x01u), ECSP_RSP_PRECONDITION);
    CHECK(fixture_configure_run(&fixture));
    CHECK_EQ(start_code(&fixture, 0x02u, 0x01u), ECSP_RSP_SUCCESS);
}

TEST(dispatch, start_while_running_returns_0x11_and_stop_leaves_the_run)
{
    psg_fixture_t fixture;

    fixture_ready(&fixture);
    CHECK(fixture_configure_run(&fixture));
    CHECK_EQ(start_code(&fixture, 0x01u, 0x01u), ECSP_RSP_SUCCESS);
    CHECK_EQ(start_code(&fixture, 0x02u, 0x01u), ECSP_RSP_BUSY);
    CHECK_EQ(start_code(&fixture, 0x03u, 0x00u), ECSP_RSP_SUCCESS);
    CHECK_EQ(psg_run_state(&fixture.run), PSG_RUN_IDLE);
}

TEST(dispatch, protocol_start_with_port_source_returns_0x12)
{
    psg_fixture_t fixture;
    static const uint8_t port_source[] = {0x01u};

    fixture_ready(&fixture);
    CHECK(fixture_configure_run(&fixture));
    CHECK_EQ(send_code(&fixture, 0x3Eu, port_source, 1u), ECSP_RSP_SUCCESS);
    CHECK_EQ(start_code(&fixture, 0x01u, 0x01u), ECSP_RSP_MODE);
}

TEST(dispatch, port_source_stop_while_running_returns_0x12)
{
    psg_fixture_t fixture;
    static const uint8_t port_source[] = {0x01u};
    uint8_t payload[2] = {0x01u, 0x00u};

    fixture_ready(&fixture);
    CHECK(fixture_configure_run(&fixture));
    CHECK_EQ(send_code(&fixture, 0x3Eu, port_source, 1u), ECSP_RSP_SUCCESS);

    /* Port start is a board event; drive the engine directly for this case. */
    CHECK_EQ(psg_run_start_verdict(&fixture.run, 1u), PSG_VERDICT_ACCEPT);
    CHECK_EQ(psg_run_start(&fixture.run), PSG_VERDICT_ACCEPT);
    CHECK_EQ(send_code(&fixture, 0x30u, payload, 2u), ECSP_RSP_MODE);
}

TEST(dispatch, current_exception_start_returns_0x15_before_source_mismatch)
{
    psg_fixture_t fixture;
    static const uint8_t port_source[] = {0x01u};

    fixture_ready(&fixture);
    CHECK(fixture_configure_run(&fixture));
    CHECK_EQ(send_code(&fixture, 0x3Eu, port_source, 1u), ECSP_RSP_SUCCESS);
    CHECK_EQ(psg_run_start(&fixture.run), PSG_VERDICT_ACCEPT);

    fixture_complete_run(&fixture, PSG_RUN_END_OVERCURRENT, PSG_CHANNEL_OVERCURRENT, 12.0f,
                         PSG_CHANNEL_STOPPED, 0.0f, 0u);
    CHECK_EQ(psg_run_state(&fixture.run), PSG_RUN_CURRENT_EXCEPTION);
    CHECK_EQ(start_code(&fixture, 0x02u, 0x01u), ECSP_RSP_SAFETY);
}

TEST(dispatch, fault_bit0_returns_0x14_for_start_and_stop)
{
    psg_fixture_t fixture;

    fixture_ready(&fixture);
    CHECK(fixture_configure_run(&fixture));
    psg_fault_set(&fixture.fault, PSG_FAULT_OVERCURRENT_LATCH);
    drain_fault_report(&fixture);
    CHECK_EQ(start_code(&fixture, 0x01u, 0x01u), ECSP_RSP_LOCKED);
    CHECK_EQ(start_code(&fixture, 0x02u, 0x00u), ECSP_RSP_LOCKED);
}

TEST(dispatch, clear_while_running_returns_0x11)
{
    psg_fixture_t fixture;

    fixture_ready(&fixture);
    CHECK(fixture_configure_run(&fixture));
    CHECK_EQ(start_code(&fixture, 0x01u, 0x01u), ECSP_RSP_SUCCESS);
    CHECK_EQ(send_code(&fixture, 0x3Du, NULL, 0u), ECSP_RSP_BUSY);
}

TEST(dispatch, address_mismatch_is_discarded_without_reply)
{
    psg_fixture_t fixture;
    uint8_t request[16];
    uint8_t response[32];
    size_t response_length = 0u;
    size_t length = ecsp_frame_encode(request, sizeof(request), 0x04u, 0x02u, ECSP_CMD_GET_FAULT, NULL, 0u);

    fixture_ready(&fixture);
    fixture_feed_frame(&fixture, request, length);
    fixture_pump(&fixture);
    CHECK(!fixture_take_tx(&fixture, response, sizeof(response), &response_length));

    length = ecsp_frame_encode(request, sizeof(request), 0x03u, 0x03u, ECSP_CMD_GET_FAULT, NULL, 0u);
    fixture_feed_frame(&fixture, request, length);
    fixture_pump(&fixture);
    CHECK(!fixture_take_tx(&fixture, response, sizeof(response), &response_length));
}

TEST(dispatch, fault_read_does_not_clear_the_fault_word)
{
    psg_fixture_t fixture;
    uint8_t response[32];
    size_t response_length = 0u;

    fixture_ready(&fixture);
    psg_fault_set(&fixture.fault, PSG_FAULT_OVERCURRENT_LATCH);
    drain_fault_report(&fixture);

    CHECK(send_command(&fixture, ECSP_CMD_GET_FAULT, NULL, 0u, response, sizeof(response), &response_length));
    CHECK_EQ(response[6], ECSP_RSP_SUCCESS);
    CHECK_EQ(response[7], PSG_FAULT_OVERCURRENT_LATCH);

    CHECK(send_command(&fixture, ECSP_CMD_GET_FAULT, NULL, 0u, response, sizeof(response), &response_length));
    CHECK_EQ(response[7], PSG_FAULT_OVERCURRENT_LATCH);
}

TEST(dispatch, reset_replies_before_the_board_reset)
{
    psg_fixture_t fixture;
    uint8_t request[16];
    uint8_t response[32];
    size_t response_length = 0u;
    size_t length = ecsp_frame_encode(request, sizeof(request), 0x03u, 0x02u, ECSP_CMD_RESET,
                                      (const uint8_t[]){0x42u}, 1u);

    fixture_ready(&fixture);
    fixture_feed_frame(&fixture, request, length);
    fixture_pump(&fixture);
    CHECK(fixture_take_tx(&fixture, response, sizeof(response), &response_length));
    CHECK_EQ(response[6], ECSP_RSP_SUCCESS);
    CHECK(!fixture.board.reset_requested);

    fixture_advance(&fixture, psg_uart_frame_duration_us((uint32_t)response_length));
    fixture_pump(&fixture);
    CHECK(fixture.board.reset_requested);
}

TEST(dispatch, dedup_replays_the_busy_rejection_after_the_run_ends)
{
    psg_fixture_t fixture;

    fixture_ready(&fixture);
    CHECK(fixture_configure_run(&fixture));
    CHECK_EQ(start_code(&fixture, 0x11u, 0x01u), ECSP_RSP_SUCCESS);
    CHECK_EQ(start_code(&fixture, 0x12u, 0x01u), ECSP_RSP_BUSY);

    fixture_complete_run(&fixture, PSG_RUN_END_NORMAL, PSG_CHANNEL_STOPPED, 8.0f, PSG_CHANNEL_STOPPED, 8.0f, 1u);
    fixture_pump(&fixture);

    CHECK_EQ(start_code(&fixture, 0x12u, 0x01u), ECSP_RSP_BUSY);
    CHECK_EQ(psg_run_state(&fixture.run), PSG_RUN_IDLE);
    CHECK_EQ(start_code(&fixture, 0x13u, 0x01u), ECSP_RSP_SUCCESS);
}

TEST(dispatch, rejects_bad_30_control_code_with_0x03)
{
    psg_fixture_t fixture;
    static const uint8_t bad_control[] = {0x01u, 0x02u};

    fixture_ready(&fixture);
    CHECK(fixture_configure_run(&fixture));
    CHECK_EQ(send_code(&fixture, 0x30u, bad_control, 2u), ECSP_RSP_BAD_PARAMETER);
}

TEST(dispatch, dedup_replays_the_busy_rejection_three_times)
{
    psg_fixture_t fixture;
    uint8_t index;

    fixture_ready(&fixture);
    CHECK(fixture_configure_run(&fixture));
    CHECK_EQ(start_code(&fixture, 0x21u, 0x01u), ECSP_RSP_SUCCESS);
    CHECK_EQ(start_code(&fixture, 0x22u, 0x01u), ECSP_RSP_BUSY);
    fixture_complete_run(&fixture, PSG_RUN_END_NORMAL, PSG_CHANNEL_STOPPED, 8.0f, PSG_CHANNEL_STOPPED, 8.0f, 1u);
    fixture_pump(&fixture);

    for (index = 0u; index < 3u; index++) {
        CHECK_EQ(start_code(&fixture, 0x22u, 0x01u), ECSP_RSP_BUSY);
        CHECK_EQ(psg_run_state(&fixture.run), PSG_RUN_IDLE);
    }
}

TEST(dispatch, status_read_shows_running_then_reports_the_fault_channel)
{
    psg_fixture_t fixture;
    uint8_t response[64];
    size_t response_length = 0u;

    fixture_ready(&fixture);
    CHECK(fixture_configure_run(&fixture));
    CHECK_EQ(start_code(&fixture, 0x31u, 0x01u), ECSP_RSP_SUCCESS);

    CHECK(send_command(&fixture, 0x31u, NULL, 0u, response, sizeof(response), &response_length));
    CHECK_EQ(response[6], ECSP_RSP_SUCCESS);
    CHECK_EQ(response[7], 0x00u); /* direct trigger */
    CHECK_EQ(response[8], PSG_CHANNEL_RUNNING);
    CHECK_EQ(response[9], PSG_CHANNEL_RUNNING);

    fixture_complete_run(&fixture, PSG_RUN_END_OVERCURRENT, PSG_CHANNEL_OVERCURRENT, 12.0f,
                         PSG_CHANNEL_STOPPED, 0.0f, 0u);
    fixture_pump(&fixture);
    CHECK(send_command(&fixture, 0x31u, NULL, 0u, response, sizeof(response), &response_length));
    CHECK_EQ(response[6], ECSP_RSP_SUCCESS);
    CHECK_EQ(response[8], PSG_CHANNEL_OVERCURRENT);
    CHECK_EQ(response[9], PSG_CHANNEL_STOPPED);
    CHECK_NEAR(ecsp_read_f32(&response[10]), 12.0f, 0.001f);
}

TEST(dispatch, reset_waits_for_its_own_reply_not_an_earlier_one)
{
    psg_fixture_t fixture;
    uint8_t request[16];
    uint8_t reset_request[16];
    uint8_t response[64];
    static const uint8_t reset_sequence = 0x33u;
    size_t response_length = 0u;
    size_t length;
    size_t reset_length;
    uint16_t fault_reply_length;

    fixture_ready(&fixture);

    /* One query reply is already on the wire when 0x07 arrives. */
    length = ecsp_frame_encode(request, sizeof(request), 0x03u, 0x02u, ECSP_CMD_GET_FAULT, NULL, 0u);
    fixture_feed_frame(&fixture, request, length);
    fixture_pump(&fixture);
    CHECK(fixture_take_tx(&fixture, response, sizeof(response), &response_length));
    fault_reply_length = (uint16_t)response_length;

    reset_length = ecsp_frame_encode(reset_request, sizeof(reset_request), 0x03u, 0x02u, ECSP_CMD_RESET,
                                     &reset_sequence, 1u);
    fixture_feed_frame(&fixture, reset_request, reset_length);
    fixture_pump(&fixture);
    /* The reset reply is queued behind the query reply, not yet on the wire. */
    CHECK(!fixture_take_tx(&fixture, response, sizeof(response), &response_length));

    fixture_advance(&fixture, psg_uart_frame_duration_us(fault_reply_length));
    fixture_pump(&fixture);
    CHECK(fixture_take_tx(&fixture, response, sizeof(response), &response_length));
    CHECK_EQ(response[6], ECSP_RSP_SUCCESS);
    CHECK(!fixture.board.reset_requested); /* the query completion must not reset */

    fixture_advance(&fixture, psg_uart_frame_duration_us((uint32_t)response_length));
    fixture_pump(&fixture);
    CHECK(fixture.board.reset_requested);
}

TEST(dispatch, bad_sequence_write_preserves_the_previous_table)
{
    psg_fixture_t fixture;
    uint8_t payload[44];
    uint8_t request[64];
    uint8_t response[64];
    size_t response_length = 0u;
    size_t length;

    fixture_ready(&fixture);
    CHECK(fixture_configure_run(&fixture));

    make_sequence_block(&payload[0], 2u, 1u, 50u, 6u, 1u, 100u, 200u, 0u, 0u, 1000u);
    make_sequence_block(&payload[22], 2u, 1u, 50u, 6u, 1u, 100u, 200u, 0u, 0u, 1000u);
    length = ecsp_frame_encode(request, sizeof(request), 0x03u, 0x02u, 0x34u, payload, 44u);
    CHECK(fixture_exchange(&fixture, request, length, response, sizeof(response), &response_length));
    CHECK_EQ(response[6], ECSP_RSP_BAD_PARAMETER);

    length = ecsp_frame_encode(request, sizeof(request), 0x03u, 0x02u, 0x35u, NULL, 0u);
    CHECK(fixture_exchange(&fixture, request, length, response, sizeof(response), &response_length));
    CHECK_EQ(response[6], ECSP_RSP_SUCCESS);
    CHECK_EQ(response[7], 1u); /* group count still 1 */
}

TEST(dispatch, sequence_rejects_zero_durations_and_gap_on_unipolar)
{
    psg_fixture_t fixture;
    uint8_t block[22];
    uint8_t request[64];
    uint8_t response[64];
    size_t response_length = 0u;
    size_t length;

    fixture_ready(&fixture);

    make_sequence_block(block, 1u, 1u, 50u, 6u, 1u, 100u, 0u, 0u, 0u, 1000u);
    length = ecsp_frame_encode(request, sizeof(request), 0x03u, 0x02u, 0x34u, block, sizeof(block));
    CHECK(fixture_exchange(&fixture, request, length, response, sizeof(response), &response_length));
    CHECK_EQ(response[6], ECSP_RSP_BAD_PARAMETER);

    make_sequence_block(block, 1u, 1u, 50u, 6u, 1u, 100u, 200u, 0u, 5000u, 1000u);
    length = ecsp_frame_encode(request, sizeof(request), 0x03u, 0x02u, 0x34u, block, sizeof(block));
    CHECK(fixture_exchange(&fixture, request, length, response, sizeof(response), &response_length));
    CHECK_EQ(response[6], ECSP_RSP_BAD_PARAMETER);
}

TEST(dispatch, rod_cycle_enable_rejects_a_burst_count_not_divisible_by_six)
{
    psg_fixture_t fixture;
    uint8_t block[22];
    uint8_t request[64];
    uint8_t response[64];
    size_t response_length = 0u;
    size_t length;

    fixture_ready(&fixture);
    make_sequence_block(block, 1u, 1u, 50u, 5u, 1u, 100u, 200u, 0u, 0u, 1000u);
    length = ecsp_frame_encode(request, sizeof(request), 0x03u, 0x02u, 0x34u, block, sizeof(block));
    CHECK(fixture_exchange(&fixture, request, length, response, sizeof(response), &response_length));
    CHECK_EQ(response[6], ECSP_RSP_SUCCESS);

    length = ecsp_frame_encode(request, sizeof(request), 0x03u, 0x02u, 0x40u, (const uint8_t[]){0x01u}, 1u);
    CHECK(fixture_exchange(&fixture, request, length, response, sizeof(response), &response_length));
    CHECK_EQ(response[6], ECSP_RSP_BAD_PARAMETER);
}

TEST(dispatch, settings_are_busy_while_running_and_safety_locked_in_exception)
{
    psg_fixture_t fixture;
    static const uint8_t mode[] = {0x01u};

    fixture_ready(&fixture);
    CHECK(fixture_configure_run(&fixture));
    CHECK_EQ(start_code(&fixture, 0x01u, 0x01u), ECSP_RSP_SUCCESS);
    CHECK_EQ(send_code(&fixture, 0x32u, mode, 1u), ECSP_RSP_BUSY);
    CHECK_EQ(send_code(&fixture, 0x0Fu, mode, 1u), ECSP_RSP_BUSY);

    fixture_complete_run(&fixture, PSG_RUN_END_OVERCURRENT, PSG_CHANNEL_OVERCURRENT, 12.0f,
                         PSG_CHANNEL_STOPPED, 0.0f, 0u);
    fixture_pump(&fixture);
    CHECK_EQ(send_code(&fixture, 0x32u, mode, 1u), ECSP_RSP_SAFETY);
    CHECK_EQ(send_code(&fixture, 0x0Fu, mode, 1u), ECSP_RSP_SAFETY);
}

TEST(dispatch, get_commands_return_power_up_defaults)
{
    psg_fixture_t fixture;
    uint8_t response[64];
    size_t response_length = 0u;

    fixture_ready(&fixture);

    CHECK(send_command(&fixture, 0x33u, NULL, 0u, response, sizeof(response), &response_length));
    CHECK_EQ(response[7], 0x00u);
    CHECK(send_command(&fixture, 0x3Fu, NULL, 0u, response, sizeof(response), &response_length));
    CHECK_EQ(response[7], 0x00u);
    CHECK(send_command(&fixture, 0x41u, NULL, 0u, response, sizeof(response), &response_length));
    CHECK_EQ(response[7], 0x00u);
    CHECK(send_command(&fixture, 0x43u, NULL, 0u, response, sizeof(response), &response_length));
    CHECK_EQ(response[7], 0x3Fu);
}
