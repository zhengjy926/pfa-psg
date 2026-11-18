/*
 * Pulse engine state and status ownership (SRS F-001..F-050, E-001..E-005,
 * board state matrix section 3.4).
 *
 * The protocol task owns the adjudication pipeline; this module supplies the
 * state-matrix verdict for each command and executes only accepted actions.
 */
#ifndef PSG_RUN_PULSE_ENGINE_H
#define PSG_RUN_PULSE_ENGINE_H

#include <stdbool.h>
#include <stdint.h>

#include "bsp/psg_board.h"
#include "run/run_config.h"
#include "svc/fault/fault.h"

typedef enum {
    PSG_RUN_IDLE = 0,
    PSG_RUN_RUNNING = 1,
    PSG_RUN_CURRENT_EXCEPTION = 2
} psg_run_state_t;

typedef enum {
    PSG_RUN_END_NORMAL = 0,
    PSG_RUN_END_STOPPED = 1,
    PSG_RUN_END_UNDERCURRENT = 2,
    PSG_RUN_END_OVERCURRENT = 3,
    PSG_RUN_END_SETTLEMENT_TIMEOUT = 4
} psg_run_end_reason_t;

typedef enum {
    PSG_VERDICT_ACCEPT = 0,
    PSG_VERDICT_BUSY = 1,
    PSG_VERDICT_MODE = 2,
    PSG_VERDICT_SAFETY = 3,
    PSG_VERDICT_LOCKED = 4,
    PSG_VERDICT_PRECONDITION = 5
} psg_verdict_t;

/* Channel output-state codes shared by 0x31 and 0x3C. */
#define PSG_CHANNEL_STOPPED 0x00u
#define PSG_CHANNEL_RUNNING 0x01u
#define PSG_CHANNEL_OVERCURRENT 0x02u
#define PSG_CHANNEL_LATCHED 0x03u
#define PSG_CHANNEL_UNDERCURRENT 0x04u

typedef struct {
    uint8_t status;
    float current;
} psg_channel_result_t;

typedef struct {
    uint8_t trigger_mode;
    uint8_t channel_status[2];
    float channel_current[2];
    uint16_t completed_triggers;
} psg_output_status_t;

typedef struct {
    psg_output_status_t status;
    psg_run_end_reason_t reason;
    uint32_t run_sequence;
    bool valid;
} psg_run_snapshot_t;

typedef struct {
    psg_output_status_t status;
    psg_run_end_reason_t reason;
    uint32_t run_sequence;
} psg_run_result_t;

typedef struct {
    psg_run_config_t config;
    psg_run_state_t state;
    psg_output_status_t status;
    psg_run_snapshot_t snapshots[2];
    uint8_t snapshot_next;

    float last_normal_current[2];
    bool has_normal_current;
    uint32_t run_sequence;
    uint16_t completed_triggers;

    bool result_pending;
    psg_run_result_t result;

    bool clear_active;
    uint64_t clear_deadline_us;

    psg_board_t board;
    psg_fault_t *fault;
} psg_run_t;

void psg_run_init(psg_run_t *run, psg_board_t board, psg_fault_t *fault);

psg_run_state_t psg_run_state(const psg_run_t *run);
const psg_run_config_t *psg_run_config(const psg_run_t *run);

/* State-matrix verdicts (ECSP 5.2 step 5, board table 3-2). */
psg_verdict_t psg_run_config_change_verdict(const psg_run_t *run);
psg_verdict_t psg_run_start_verdict(const psg_run_t *run, uint8_t request_source);
psg_verdict_t psg_run_stop_verdict(const psg_run_t *run, uint8_t request_source);
psg_verdict_t psg_run_clear_verdict(const psg_run_t *run);

/* Commit a validated configuration candidate. */
void psg_run_commit_trigger_mode(psg_run_t *run, uint8_t value);
void psg_run_commit_start_source(psg_run_t *run, uint8_t value);
void psg_run_commit_rod_cycle(psg_run_t *run, bool enabled);
void psg_run_commit_rod_select(psg_run_t *run, uint8_t value);
void psg_run_commit_r_wave(psg_run_t *run, const psg_r_wave_t *r_wave);
void psg_run_commit_thresholds(psg_run_t *run, const psg_thresholds_t *thresholds);
void psg_run_commit_voltages(psg_run_t *run, const psg_voltages_t *voltages);
void psg_run_commit_sequence(psg_run_t *run, const psg_sequence_t *sequence);

/* Execute an accepted start (runs the pre-output latch self-check). */
psg_verdict_t psg_run_start(psg_run_t *run);
void psg_run_stop(psg_run_t *run);

/* Publish the end of one run: updates live status, snapshots the result and
 * makes it available to 0x31 / 0x3C. */
void psg_run_complete(psg_run_t *run, psg_run_end_reason_t reason, const psg_channel_result_t *channels,
                      uint16_t completed_rounds);

/* 0x3D: pulse the latch reset, wait PRM-512, then re-read. */
void psg_run_clear_begin(psg_run_t *run, uint64_t now_us);
bool psg_run_clear_due(const psg_run_t *run, uint64_t now_us);
bool psg_run_clear_finish(psg_run_t *run);

/* I-012 duplicate handshake: terminate the run and invalidate sequence,
 * voltage and threshold configuration; anomaly records are retained. */
void psg_run_reset_after_handshake(psg_run_t *run);

bool psg_run_take_result(psg_run_t *run, psg_run_result_t *result);
const psg_output_status_t *psg_run_status(const psg_run_t *run);
const psg_run_snapshot_t *psg_run_snapshot(const psg_run_t *run, uint8_t slot);
uint8_t psg_run_fault_word(const psg_run_t *run);

#endif /* PSG_RUN_PULSE_ENGINE_H */
