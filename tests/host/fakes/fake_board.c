#include "fakes/fake_board.h"

static void fake_request_reset(void *context)
{
    fake_board_t *fake = (fake_board_t *)context;

    fake->reset_requested = true;
}

static bool fake_overcurrent_active(void *context)
{
    const fake_board_t *fake = (const fake_board_t *)context;

    return fake->overcurrent;
}

static bool fake_latch_active(void *context)
{
    const fake_board_t *fake = (const fake_board_t *)context;

    return fake->latch;
}

static void fake_latch_reset_pulse(void *context)
{
    fake_board_t *fake = (fake_board_t *)context;

    fake->latch_reset_count++;
    if (fake->latch_clears_on_reset) {
        fake->latch = false;
    }
}

static void fake_set_running_indicator(void *context, bool on)
{
    fake_board_t *fake = (fake_board_t *)context;

    fake->running_indicator = on;
}

static const psg_board_ops_t g_board_ops = {
    .request_reset = fake_request_reset,
    .overcurrent_active = fake_overcurrent_active,
    .latch_active = fake_latch_active,
    .latch_reset_pulse = fake_latch_reset_pulse,
    .set_running_indicator = fake_set_running_indicator,
};

void fake_board_init(fake_board_t *fake)
{
    fake->base.ops = &g_board_ops;
    fake->base.context = fake;
    fake->reset_requested = false;
    fake->overcurrent = false;
    fake->latch = false;
    fake->latch_clears_on_reset = true;
    fake->latch_reset_count = 0u;
    fake->running_indicator = false;
}
