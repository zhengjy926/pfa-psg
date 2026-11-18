#include "psg_fixture.h"

#include <string.h>

#include "app/ecsp/ecsp_frame.h"
#include "drv/timebase/timebase.h"

static void fixture_pump_uart(psg_fixture_t *fixture)
{
    uint8_t values[64];
    uint32_t stamps[64];

    for (;;) {
        uint32_t gaps = uart_take_gaps(&fixture->uart.base);
        size_t count = uart_read(&fixture->uart.base, values, stamps, 64u);
        psg_ecsp_feed(&fixture->ecsp, values, stamps, count, gaps);
        if (count < 64u) {
            break;
        }
    }
    psg_ecsp_step(&fixture->ecsp, fixture->clock.now_us);
}

void fixture_init(psg_fixture_t *fixture)
{
    psg_ecsp_config_t config;

    (void)memset(fixture, 0, sizeof(*fixture));
    fake_timebase_init(&fixture->clock, 0u);
    fake_uart_init(&fixture->uart, &fixture->clock);
    fake_board_init(&fixture->board);
    fake_led_init(&fixture->led);
    fake_flash_init(&fixture->flash);
    psg_fault_init(&fixture->fault);
    identity_init(&fixture->identity, &fixture->flash.base);
    psg_run_init(&fixture->run, fixture->board.base, &fixture->fault);

    config.uart = &fixture->uart.base;
    config.board = fixture->board.base;
    config.run = &fixture->run;
    config.identity = &fixture->identity;
    config.fault = &fixture->fault;
    config.product_address = 0x03u;
    config.board_address = 0x02u;
    psg_ecsp_init(&fixture->ecsp, &config, fixture->clock.now_us);
    fixture->command_sequence = 1u;
}

void fixture_pump(psg_fixture_t *fixture)
{
    fixture_pump_uart(fixture);
}

void fixture_advance(psg_fixture_t *fixture, uint64_t delta_us)
{
    fake_timebase_advance(&fixture->clock, delta_us);
}

void fixture_feed_bytes(psg_fixture_t *fixture, const uint8_t *bytes, size_t count, uint32_t timestamp_us)
{
    fake_uart_push_bytes(&fixture->uart, bytes, count, timestamp_us);
}

void fixture_feed_frame(psg_fixture_t *fixture, const uint8_t *frame, size_t length)
{
    fixture_feed_bytes(fixture, frame, length, (uint32_t)fixture->clock.now_us);
}

bool fixture_take_tx(psg_fixture_t *fixture, uint8_t *out, size_t capacity, size_t *length)
{
    return fake_uart_take_tx(&fixture->uart, out, capacity, length);
}

bool fixture_exchange(psg_fixture_t *fixture, const uint8_t *request, size_t request_length, uint8_t *response,
                      size_t capacity, size_t *response_length)
{
    size_t length = 0u;

    fixture_feed_frame(fixture, request, request_length);
    fixture_pump(fixture);
    if (!fixture_take_tx(fixture, response, capacity, &length)) {
        return false;
    }
    if (response_length != NULL) {
        *response_length = length;
    }
    fixture_advance(fixture, psg_uart_frame_duration_us((uint32_t)length));
    fixture_pump(fixture);
    return true;
}

void fixture_ready(psg_fixture_t *fixture)
{
    static const uint8_t handshake_request[] = {0xFAu, 0x09u, 0x00u, 0x03u, 0x01u,
                                                0x02u, 0x88u, 0x50u, 0x0Du};
    uint8_t response[32];
    size_t response_length = 0u;

    fixture_init(fixture);

    /* Power-up handshake report is sent immediately (<= PRM-116). */
    fixture_pump(fixture);
    if (fixture_take_tx(fixture, fixture->boot_report, sizeof(fixture->boot_report),
                        &fixture->boot_report_length)) {
        fixture_advance(fixture,
                        psg_uart_frame_duration_us((uint32_t)fixture->boot_report_length));
        fixture_pump(fixture);
    }

    (void)fixture_exchange(fixture, handshake_request, sizeof(handshake_request), response, sizeof(response),
                           &response_length);
}

bool fixture_configure_run(psg_fixture_t *fixture)
{
    static const uint8_t set_voltages[] = {0xFAu, 0x0Du, 0x00u, 0x03u, 0x3Au, 0x02u,
                                           0x08u, 0x07u, 0x08u, 0x07u, 0xE8u, 0x59u, 0x0Du};
    uint8_t request[64];
    uint8_t response[32];
    size_t response_length = 0u;
    size_t length;
    uint8_t payload[22];
    uint8_t thresholds[16];

    if (!fixture_exchange(fixture, set_voltages, sizeof(set_voltages), response, sizeof(response),
                          &response_length)) {
        return false;
    }

    ecsp_write_f32(&thresholds[0], 10.0f);
    ecsp_write_f32(&thresholds[4], 10.0f);
    ecsp_write_f32(&thresholds[8], 1.0f);
    ecsp_write_f32(&thresholds[12], 1.0f);
    length = ecsp_frame_encode(request, sizeof(request), 0x03u, 0x02u, 0x38u, thresholds, 16u);
    if (length == 0u) {
        return false;
    }
    if (!fixture_exchange(fixture, request, length, response, sizeof(response), &response_length)) {
        return false;
    }

    (void)memset(payload, 0, sizeof(payload));
    payload[0] = 1u;    /* group count */
    payload[1] = 1u;    /* group index */
    ecsp_write_u16(&payload[2], 50u);    /* group interval 50 ms */
    ecsp_write_u16(&payload[4], 6u);     /* bursts per group */
    ecsp_write_u16(&payload[6], 1u);     /* waves per burst */
    ecsp_write_u16(&payload[8], 100u);   /* burst interval 1 ms */
    ecsp_write_u16(&payload[10], 200u);  /* positive pulse 200 ns */
    ecsp_write_u16(&payload[12], 0u);    /* negative pulse 0 */
    ecsp_write_u32(&payload[14], 0u);    /* inter-polarity gap */
    ecsp_write_u32(&payload[18], 1000u); /* pulse interval 1 us */
    length = ecsp_frame_encode(request, sizeof(request), 0x03u, 0x02u, 0x34u, payload, sizeof(payload));
    if (length == 0u) {
        return false;
    }
    if (!fixture_exchange(fixture, request, length, response, sizeof(response), &response_length)) {
        return false;
    }
    return response_length >= 7u && response[6] == 0x00u;
}

bool fixture_start_run(psg_fixture_t *fixture)
{
    uint8_t request[16];
    uint8_t response[32];
    uint8_t payload[2];
    size_t response_length = 0u;
    size_t length;

    payload[0] = fixture->command_sequence;
    payload[1] = 1u;
    fixture->command_sequence++;
    length = ecsp_frame_encode(request, sizeof(request), 0x03u, 0x02u, 0x30u, payload, 2u);

    if ((length == 0u) || (!fixture_exchange(fixture, request, length, response, sizeof(response),
                                             &response_length))) {
        return false;
    }
    return response_length >= 7u && response[6] == 0x00u;
}

void fixture_complete_run(psg_fixture_t *fixture, psg_run_end_reason_t reason, uint8_t pos_status, float pos_current,
                          uint8_t neg_status, float neg_current, uint16_t rounds)
{
    psg_channel_result_t channels[2];

    channels[0].status = pos_status;
    channels[0].current = pos_current;
    channels[1].status = neg_status;
    channels[1].current = neg_current;
    psg_run_complete(&fixture->run, reason, channels, rounds);
}
