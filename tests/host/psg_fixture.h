#ifndef PSG_TESTS_HOST_PSG_FIXTURE_H
#define PSG_TESTS_HOST_PSG_FIXTURE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "app/psg_ecsp.h"
#include "fakes/fake_board.h"
#include "fakes/fake_flash.h"
#include "fakes/fake_led.h"
#include "fakes/fake_timebase.h"
#include "fakes/fake_uart.h"

typedef struct {
    fake_timebase_t clock;
    fake_uart_t uart;
    fake_board_t board;
    fake_led_t led;
    fake_flash_t flash;
    psg_fault_t fault;
    identity_t identity;
    psg_run_t run;
    psg_ecsp_t ecsp;

    uint8_t boot_report[16];
    size_t boot_report_length;
    uint8_t command_sequence;
} psg_fixture_t;

void fixture_init(psg_fixture_t *fixture);

/* Run the UART-task hand-off and the protocol task once. */
void fixture_pump(psg_fixture_t *fixture);

void fixture_advance(psg_fixture_t *fixture, uint64_t delta_us);
void fixture_feed_bytes(psg_fixture_t *fixture, const uint8_t *bytes, size_t count, uint32_t timestamp_us);
void fixture_feed_frame(psg_fixture_t *fixture, const uint8_t *frame, size_t length);

bool fixture_take_tx(psg_fixture_t *fixture, uint8_t *out, size_t capacity, size_t *length);

/* Feed a request and return the next transmitted frame after completing it. */
bool fixture_exchange(psg_fixture_t *fixture, const uint8_t *request, size_t request_length, uint8_t *response,
                      size_t capacity, size_t *response_length);

/* Power-up: consume the first handshake report and establish the session. */
void fixture_ready(psg_fixture_t *fixture);

/* Configure voltages, thresholds and one sequence group. */
bool fixture_configure_run(psg_fixture_t *fixture);

/* Start a direct-trigger run and complete it with the given channel results. */
bool fixture_start_run(psg_fixture_t *fixture);
void fixture_complete_run(psg_fixture_t *fixture, psg_run_end_reason_t reason, uint8_t pos_status, float pos_current,
                          uint8_t neg_status, float neg_current, uint16_t rounds);

#endif /* PSG_TESTS_HOST_PSG_FIXTURE_H */
