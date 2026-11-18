#ifndef PSG_TESTS_HOST_FAKES_FAKE_BOARD_H
#define PSG_TESTS_HOST_FAKES_FAKE_BOARD_H

#include <stdbool.h>
#include <stdint.h>

#include "bsp/psg_board.h"

typedef struct {
    psg_board_t base;
    bool reset_requested;
    bool overcurrent;
    bool latch;
    bool latch_clears_on_reset;
    uint32_t latch_reset_count;
    bool running_indicator;
} fake_board_t;

void fake_board_init(fake_board_t *fake);

#endif /* PSG_TESTS_HOST_FAKES_FAKE_BOARD_H */
