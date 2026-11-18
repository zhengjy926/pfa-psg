/*
 * Board support boundary.  The run layer asks the board only for actions it
 * cannot perform itself: reset, protection evidence, and the running lamp.
 */
#ifndef PSG_BSP_PSG_BOARD_H
#define PSG_BSP_PSG_BOARD_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    void (*request_reset)(void *context);
    bool (*overcurrent_active)(void *context);
    bool (*latch_active)(void *context);
    void (*latch_reset_pulse)(void *context);
    void (*set_running_indicator)(void *context, bool on);
} psg_board_ops_t;

typedef struct {
    const psg_board_ops_t *ops;
    void *context;
} psg_board_t;

static inline void psg_board_request_reset(const psg_board_t *board)
{
    board->ops->request_reset(board->context);
}

static inline bool psg_board_overcurrent_active(const psg_board_t *board)
{
    return board->ops->overcurrent_active(board->context);
}

static inline bool psg_board_latch_active(const psg_board_t *board)
{
    return board->ops->latch_active(board->context);
}

static inline void psg_board_latch_reset_pulse(const psg_board_t *board)
{
    board->ops->latch_reset_pulse(board->context);
}

static inline void psg_board_set_running_indicator(const psg_board_t *board, bool on)
{
    board->ops->set_running_indicator(board->context, on);
}

#endif /* PSG_BSP_PSG_BOARD_H */
