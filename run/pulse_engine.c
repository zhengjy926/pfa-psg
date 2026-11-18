#include "run/pulse_engine.h"

#include <stddef.h>

#include "drv/timebase/timebase.h"

void psg_run_init(psg_run_t *run, psg_board_t board, psg_fault_t *fault)
{
    psg_config_defaults(&run->config);
    run->state = PSG_RUN_IDLE;
    run->status.trigger_mode = run->config.trigger_mode;
    run->status.channel_status[0] = PSG_CHANNEL_STOPPED;
    run->status.channel_status[1] = PSG_CHANNEL_STOPPED;
    run->status.channel_current[0] = 0.0f;
    run->status.channel_current[1] = 0.0f;
    run->status.completed_triggers = 0u;
    run->snapshot_next = 0u;
    run->last_normal_current[0] = 0.0f;
    run->last_normal_current[1] = 0.0f;
    run->has_normal_current = false;
    run->run_sequence = 0u;
    run->completed_triggers = 0u;
    run->result_pending = false;
    run->clear_active = false;
    run->clear_deadline_us = 0u;
    run->board = board;
    run->fault = fault;
}

psg_run_state_t psg_run_state(const psg_run_t *run)
{
    return run->state;
}

const psg_run_config_t *psg_run_config(const psg_run_t *run)
{
    return &run->config;
}

static bool start_configured(const psg_run_t *run)
{
    if ((run->config.voltages.voltage_pos == 0u) || (run->config.voltages.voltage_neg == 0u)) {
        return false;
    }
    if ((run->config.thresholds.overcurrent_pos <= 0.0f) || (run->config.thresholds.overcurrent_neg <= 0.0f) ||
        (run->config.thresholds.undercurrent_pos <= 0.0f) || (run->config.thresholds.undercurrent_neg <= 0.0f)) {
        return false;
    }
    if (run->config.sequence.group_count == 0u) {
        return false;
    }
    if ((run->config.trigger_mode == 1u) && (!run->config.r_wave_configured)) {
        return false;
    }
    return true;
}

psg_verdict_t psg_run_config_change_verdict(const psg_run_t *run)
{
    if (run->state == PSG_RUN_CURRENT_EXCEPTION) {
        return PSG_VERDICT_SAFETY;
    }
    if (run->state == PSG_RUN_RUNNING) {
        return PSG_VERDICT_BUSY;
    }
    return PSG_VERDICT_ACCEPT;
}

psg_verdict_t psg_run_start_verdict(const psg_run_t *run, uint8_t request_source)
{
    if (run->state == PSG_RUN_CURRENT_EXCEPTION) {
        return PSG_VERDICT_SAFETY;
    }
    if (psg_fault_word(run->fault) != 0u) {
        return PSG_VERDICT_LOCKED;
    }
    if (request_source != run->config.start_source) {
        return PSG_VERDICT_MODE;
    }
    if (run->state == PSG_RUN_RUNNING) {
        return PSG_VERDICT_BUSY;
    }
    if (psg_board_overcurrent_active(&run->board)) {
        return PSG_VERDICT_SAFETY;
    }
    if (!start_configured(run)) {
        return PSG_VERDICT_PRECONDITION;
    }
    return PSG_VERDICT_ACCEPT;
}

psg_verdict_t psg_run_stop_verdict(const psg_run_t *run, uint8_t request_source)
{
    if (run->state == PSG_RUN_CURRENT_EXCEPTION) {
        return PSG_VERDICT_SAFETY;
    }
    if (psg_fault_word(run->fault) != 0u) {
        return PSG_VERDICT_LOCKED;
    }
    if ((run->state == PSG_RUN_RUNNING) && (request_source != run->config.start_source)) {
        return PSG_VERDICT_MODE;
    }
    return PSG_VERDICT_ACCEPT;
}

psg_verdict_t psg_run_clear_verdict(const psg_run_t *run)
{
    if (run->state == PSG_RUN_RUNNING) {
        return PSG_VERDICT_BUSY;
    }
    return PSG_VERDICT_ACCEPT;
}

void psg_run_commit_trigger_mode(psg_run_t *run, uint8_t value)
{
    run->config.trigger_mode = value;
    run->status.trigger_mode = value;
}

void psg_run_commit_start_source(psg_run_t *run, uint8_t value)
{
    run->config.start_source = value;
}

void psg_run_commit_rod_cycle(psg_run_t *run, bool enabled)
{
    run->config.rod_cycle_enabled = enabled;
}

void psg_run_commit_rod_select(psg_run_t *run, uint8_t value)
{
    run->config.rod_select = value;
}

void psg_run_commit_r_wave(psg_run_t *run, const psg_r_wave_t *r_wave)
{
    run->config.r_wave = *r_wave;
    run->config.r_wave_configured = true;
}

void psg_run_commit_thresholds(psg_run_t *run, const psg_thresholds_t *thresholds)
{
    run->config.thresholds = *thresholds;
}

void psg_run_commit_voltages(psg_run_t *run, const psg_voltages_t *voltages)
{
    run->config.voltages = *voltages;
}

void psg_run_commit_sequence(psg_run_t *run, const psg_sequence_t *sequence)
{
    run->config.sequence = *sequence;
}

psg_verdict_t psg_run_start(psg_run_t *run)
{
    uint8_t channel;

    /* Pre-output latch self-check (E-003).  The full 5 ms clear/confirm flow
     * belongs to the over-current ticket; until then an asserted latch is
     * treated as a lock so no run can start against it. */
    if (psg_board_latch_active(&run->board)) {
        psg_board_latch_reset_pulse(&run->board);
        psg_fault_set(run->fault, PSG_FAULT_OVERCURRENT_LATCH);
        return PSG_VERDICT_LOCKED;
    }

    run->state = PSG_RUN_RUNNING;
    run->run_sequence++;
    run->completed_triggers = 0u;
    run->status.trigger_mode = run->config.trigger_mode;
    for (channel = 0u; channel < 2u; channel++) {
        run->status.channel_status[channel] = PSG_CHANNEL_RUNNING;
        run->status.channel_current[channel] = 0.0f;
    }
    run->status.completed_triggers = 0u;
    psg_board_set_running_indicator(&run->board, true);
    return PSG_VERDICT_ACCEPT;
}

void psg_run_stop(psg_run_t *run)
{
    run->state = PSG_RUN_IDLE;
    run->status.channel_status[0] = PSG_CHANNEL_STOPPED;
    run->status.channel_status[1] = PSG_CHANNEL_STOPPED;
    psg_board_set_running_indicator(&run->board, false);
}

static void publish_result(psg_run_t *run, psg_run_end_reason_t reason)
{
    psg_run_snapshot_t *snapshot = &run->snapshots[run->snapshot_next];

    snapshot->status = run->status;
    snapshot->reason = reason;
    snapshot->run_sequence = run->run_sequence;
    snapshot->valid = true;
    run->snapshot_next = (uint8_t)((run->snapshot_next + 1u) % 2u);

    run->result.status = run->status;
    run->result.reason = reason;
    run->result.run_sequence = run->run_sequence;
    run->result_pending = true;
}

void psg_run_complete(psg_run_t *run, psg_run_end_reason_t reason, const psg_channel_result_t *channels,
                      uint16_t completed_rounds)
{
    uint8_t channel;

    for (channel = 0u; channel < 2u; channel++) {
        run->status.channel_current[channel] = channels[channel].current;
        switch (reason) {
        case PSG_RUN_END_NORMAL:
        case PSG_RUN_END_STOPPED:
        case PSG_RUN_END_SETTLEMENT_TIMEOUT:
            run->status.channel_status[channel] = PSG_CHANNEL_STOPPED;
            break;
        case PSG_RUN_END_UNDERCURRENT:
        case PSG_RUN_END_OVERCURRENT:
            run->status.channel_status[channel] = channels[channel].status;
            break;
        default:
            run->status.channel_status[channel] = PSG_CHANNEL_STOPPED;
            break;
        }
    }

    switch (reason) {
    case PSG_RUN_END_NORMAL:
        run->completed_triggers = completed_rounds;
        run->status.completed_triggers = completed_rounds;
        run->last_normal_current[0] = run->status.channel_current[0];
        run->last_normal_current[1] = run->status.channel_current[1];
        run->has_normal_current = true;
        run->state = PSG_RUN_IDLE;
        break;
    case PSG_RUN_END_UNDERCURRENT:
    case PSG_RUN_END_OVERCURRENT:
        run->state = PSG_RUN_CURRENT_EXCEPTION;
        break;
    case PSG_RUN_END_STOPPED:
    case PSG_RUN_END_SETTLEMENT_TIMEOUT:
    default:
        run->state = PSG_RUN_IDLE;
        break;
    }

    psg_board_set_running_indicator(&run->board, false);
    publish_result(run, reason);
}

void psg_run_clear_begin(psg_run_t *run, uint64_t now_us)
{
    psg_board_latch_reset_pulse(&run->board);
    run->clear_active = true;
    run->clear_deadline_us = now_us + 5000u; /* PRM-512 */
}

bool psg_run_clear_due(const psg_run_t *run, uint64_t now_us)
{
    return run->clear_active && psg_time_reached(now_us, run->clear_deadline_us);
}

bool psg_run_clear_finish(psg_run_t *run)
{
    run->clear_active = false;
    if (psg_board_latch_active(&run->board)) {
        /* I-021: still latched -> bit0 set and 0x20. */
        psg_fault_set(run->fault, PSG_FAULT_OVERCURRENT_LATCH);
        return false;
    }

    psg_fault_clear(run->fault, PSG_FAULT_OVERCURRENT_LATCH);
    run->state = PSG_RUN_IDLE;
    run->status.channel_status[0] = PSG_CHANNEL_STOPPED;
    run->status.channel_status[1] = PSG_CHANNEL_STOPPED;
    if (run->has_normal_current) {
        run->status.channel_current[0] = run->last_normal_current[0];
        run->status.channel_current[1] = run->last_normal_current[1];
    } else {
        run->status.channel_current[0] = 0.0f;
        run->status.channel_current[1] = 0.0f;
    }
    return true;
}

void psg_run_reset_after_handshake(psg_run_t *run)
{
    if (run->state == PSG_RUN_RUNNING) {
        run->state = PSG_RUN_IDLE;
        psg_board_set_running_indicator(&run->board, false);
    }
    run->config.sequence.group_count = 0u;
    run->config.voltages.voltage_pos = 0u;
    run->config.voltages.voltage_neg = 0u;
    run->config.thresholds.overcurrent_pos = 0.0f;
    run->config.thresholds.overcurrent_neg = 0.0f;
    run->config.thresholds.undercurrent_pos = 0.0f;
    run->config.thresholds.undercurrent_neg = 0.0f;

    if (run->state != PSG_RUN_CURRENT_EXCEPTION) {
        run->status.channel_status[0] = PSG_CHANNEL_STOPPED;
        run->status.channel_status[1] = PSG_CHANNEL_STOPPED;
        run->status.channel_current[0] = 0.0f;
        run->status.channel_current[1] = 0.0f;
    }
    run->status.completed_triggers = 0u;
    run->completed_triggers = 0u;
    run->result_pending = false;
}

bool psg_run_take_result(psg_run_t *run, psg_run_result_t *result)
{
    if (!run->result_pending) {
        return false;
    }
    *result = run->result;
    run->result_pending = false;
    return true;
}

const psg_output_status_t *psg_run_status(const psg_run_t *run)
{
    return &run->status;
}

const psg_run_snapshot_t *psg_run_snapshot(const psg_run_t *run, uint8_t slot)
{
    return (slot < 2u) ? &run->snapshots[slot] : NULL;
}

uint8_t psg_run_fault_word(const psg_run_t *run)
{
    return psg_fault_word(run->fault);
}
