#include "test_support.h"

#include "app/ecsp/ecsp_frame.h"
#include "psg_fixture.h"

static bool clear_anomaly(psg_fixture_t *fixture, uint8_t *response, size_t capacity, size_t *response_length)
{
    uint8_t request[16];
    size_t length = ecsp_frame_encode(request, sizeof(request), 0x03u, 0x02u, 0x3Du, NULL, 0u);

    fixture_feed_frame(fixture, request, length);
    fixture_pump(fixture);
    CHECK(!fixture_take_tx(fixture, response, capacity, response_length));

    fixture_advance(fixture, 5000u); /* PRM-512 latch clear confirmation */
    fixture_pump(fixture);
    return fixture_take_tx(fixture, response, capacity, response_length);
}

TEST(run, normal_completion_publishes_status_and_snapshot)
{
    psg_fixture_t fixture;
    const psg_output_status_t *status;
    const psg_run_snapshot_t *snapshot;

    fixture_ready(&fixture);
    CHECK(fixture_configure_run(&fixture));
    CHECK(fixture_start_run(&fixture));
    CHECK_EQ(psg_run_state(&fixture.run), PSG_RUN_RUNNING);
    CHECK(fixture.board.running_indicator);

    fixture_complete_run(&fixture, PSG_RUN_END_NORMAL, PSG_CHANNEL_STOPPED, 7.0f, PSG_CHANNEL_STOPPED, 8.0f, 1u);
    CHECK_EQ(psg_run_state(&fixture.run), PSG_RUN_IDLE);
    CHECK(!fixture.board.running_indicator);

    status = psg_run_status(&fixture.run);
    CHECK_EQ(status->channel_status[0], PSG_CHANNEL_STOPPED);
    CHECK_EQ(status->channel_status[1], PSG_CHANNEL_STOPPED);
    CHECK_NEAR(status->channel_current[0], 7.0f, 0.001f);
    CHECK_NEAR(status->channel_current[1], 8.0f, 0.001f);
    CHECK_EQ(status->completed_triggers, 1u);

    snapshot = psg_run_snapshot(&fixture.run, 0u);
    CHECK(snapshot != NULL);
    CHECK(snapshot->valid);
    CHECK_EQ(snapshot->reason, PSG_RUN_END_NORMAL);
    CHECK_EQ(snapshot->run_sequence, 1u);
    CHECK_NEAR(snapshot->status.channel_current[0], 7.0f, 0.001f);
}

TEST(run, snapshot_ring_overwrites_the_oldest_slot)
{
    psg_fixture_t fixture;

    fixture_ready(&fixture);
    CHECK(fixture_configure_run(&fixture));

    CHECK(fixture_start_run(&fixture));
    fixture_complete_run(&fixture, PSG_RUN_END_NORMAL, PSG_CHANNEL_STOPPED, 1.0f, PSG_CHANNEL_STOPPED, 1.0f, 1u);
    CHECK(fixture_start_run(&fixture));
    fixture_complete_run(&fixture, PSG_RUN_END_NORMAL, PSG_CHANNEL_STOPPED, 2.0f, PSG_CHANNEL_STOPPED, 2.0f, 1u);
    CHECK(fixture_start_run(&fixture));
    fixture_complete_run(&fixture, PSG_RUN_END_NORMAL, PSG_CHANNEL_STOPPED, 3.0f, PSG_CHANNEL_STOPPED, 3.0f, 1u);

    CHECK_EQ(psg_run_snapshot(&fixture.run, 0u)->run_sequence, 3u);
    CHECK_NEAR(psg_run_snapshot(&fixture.run, 0u)->status.channel_current[0], 3.0f, 0.001f);
    CHECK_EQ(psg_run_snapshot(&fixture.run, 1u)->run_sequence, 2u);
    CHECK_NEAR(psg_run_snapshot(&fixture.run, 1u)->status.channel_current[0], 2.0f, 0.001f);
    CHECK_EQ(psg_run_snapshot(&fixture.run, 2u), NULL);
}

TEST(run, undercurrent_enters_current_exception_and_clear_restores_normal_reading)
{
    psg_fixture_t fixture;
    uint8_t response[32];
    size_t response_length = 0u;

    fixture_ready(&fixture);
    CHECK(fixture_configure_run(&fixture));
    CHECK(fixture_start_run(&fixture));
    fixture_complete_run(&fixture, PSG_RUN_END_NORMAL, PSG_CHANNEL_STOPPED, 7.0f, PSG_CHANNEL_STOPPED, 8.0f, 1u);

    CHECK(fixture_start_run(&fixture));
    fixture_complete_run(&fixture, PSG_RUN_END_UNDERCURRENT, PSG_CHANNEL_UNDERCURRENT, 0.5f, PSG_CHANNEL_STOPPED,
                         0.0f, 0u);
    CHECK_EQ(psg_run_state(&fixture.run), PSG_RUN_CURRENT_EXCEPTION);
    CHECK_EQ(psg_run_status(&fixture.run)->channel_status[0], PSG_CHANNEL_UNDERCURRENT);
    CHECK_EQ(psg_run_status(&fixture.run)->channel_status[1], PSG_CHANNEL_STOPPED);

    CHECK(clear_anomaly(&fixture, response, sizeof(response), &response_length));
    CHECK_EQ(response[6], 0x00u);
    CHECK_EQ(psg_run_state(&fixture.run), PSG_RUN_IDLE);
    CHECK_EQ(psg_run_status(&fixture.run)->channel_status[0], PSG_CHANNEL_STOPPED);
    CHECK_EQ(psg_run_status(&fixture.run)->channel_status[1], PSG_CHANNEL_STOPPED);
    CHECK_NEAR(psg_run_status(&fixture.run)->channel_current[0], 7.0f, 0.001f);
    CHECK_NEAR(psg_run_status(&fixture.run)->channel_current[1], 8.0f, 0.001f);
    CHECK_EQ(psg_run_status(&fixture.run)->completed_triggers, 0u);
}

TEST(run, idle_clear_retains_completed_triggers)
{
    psg_fixture_t fixture;
    uint8_t response[32];
    size_t response_length = 0u;

    fixture_ready(&fixture);
    CHECK(fixture_configure_run(&fixture));
    CHECK(fixture_start_run(&fixture));
    fixture_complete_run(&fixture, PSG_RUN_END_NORMAL, PSG_CHANNEL_STOPPED, 7.0f, PSG_CHANNEL_STOPPED, 8.0f, 1u);
    CHECK_EQ(psg_run_status(&fixture.run)->completed_triggers, 1u);

    CHECK(clear_anomaly(&fixture, response, sizeof(response), &response_length));
    CHECK_EQ(response[6], 0x00u);
    CHECK_EQ(psg_run_status(&fixture.run)->completed_triggers, 1u);
}

TEST(run, failed_clear_sets_bit0_and_replies_0x20)
{
    psg_fixture_t fixture;
    uint8_t response[32];
    size_t response_length = 0u;

    fixture_ready(&fixture);
    CHECK(fixture_configure_run(&fixture));
    CHECK(fixture_start_run(&fixture));
    fixture_complete_run(&fixture, PSG_RUN_END_OVERCURRENT, PSG_CHANNEL_OVERCURRENT, 12.0f,
                         PSG_CHANNEL_STOPPED, 0.0f, 0u);

    fixture.board.latch = true;
    fixture.board.latch_clears_on_reset = false;
    CHECK(clear_anomaly(&fixture, response, sizeof(response), &response_length));
    CHECK_EQ(response[6], 0x20u);
    CHECK_EQ(psg_run_fault_word(&fixture.run), PSG_FAULT_OVERCURRENT_LATCH);
    CHECK_EQ(psg_run_state(&fixture.run), PSG_RUN_CURRENT_EXCEPTION);
}

TEST(run, asserted_latch_at_start_returns_0x14_and_sets_bit0)
{
    psg_fixture_t fixture;
    uint8_t request[16];
    uint8_t response[32];
    size_t response_length = 0u;
    size_t length;

    fixture_ready(&fixture);
    CHECK(fixture_configure_run(&fixture));
    fixture.board.latch = true;

    length = ecsp_frame_encode(request, sizeof(request), 0x03u, 0x02u, 0x30u,
                               (const uint8_t[]){0x50u, 0x01u}, 2u);
    CHECK(fixture_exchange(&fixture, request, length, response, sizeof(response), &response_length));
    CHECK_EQ(response[6], 0x14u);
    CHECK_EQ(psg_run_fault_word(&fixture.run), PSG_FAULT_OVERCURRENT_LATCH);
    CHECK_EQ(psg_run_state(&fixture.run), PSG_RUN_IDLE);
    CHECK_EQ(fixture.board.latch_reset_count, 1u);
}

TEST(run, settled_average_is_kept_per_channel)
{
    psg_fixture_t fixture;

    fixture_ready(&fixture);
    CHECK(fixture_configure_run(&fixture));
    CHECK(fixture_start_run(&fixture));
    fixture_complete_run(&fixture, PSG_RUN_END_NORMAL, PSG_CHANNEL_STOPPED, 17.2f, PSG_CHANNEL_STOPPED, 16.4f, 1u);

    CHECK_NEAR(psg_run_status(&fixture.run)->channel_current[0], 17.2f, 0.001f);
    CHECK_NEAR(psg_run_status(&fixture.run)->channel_current[1], 16.4f, 0.001f);
}
