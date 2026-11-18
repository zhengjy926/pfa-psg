/*
 * Committed run configuration and payload validation (SRS F-016, F-019, F-020,
 * F-023, I-012..I-020, E-007).
 *
 * Validation is pure: a rejected request never touches the committed config.
 * The protocol layer validates first (0x02/0x03), asks the run layer for a
 * state verdict, and only then commits the parsed candidate.
 */
#ifndef PSG_RUN_RUN_CONFIG_H
#define PSG_RUN_RUN_CONFIG_H

#include <stdbool.h>
#include <stdint.h>

#include "shared/psg_config.h"

#define PSG_GROUP_MAX 20u
#define PSG_GROUP_ENCODED_BYTES 22u

typedef enum {
    PSG_CONFIG_OK = 0,
    PSG_CONFIG_BAD_LENGTH = 1,
    PSG_CONFIG_BAD_PARAMETER = 2
} psg_config_result_t;

typedef struct {
    uint8_t group_count_field;
    uint8_t group_index;
    uint16_t group_interval_ms;
    uint16_t burst_count;
    uint16_t wave_count;
    uint16_t burst_interval_10us;
    uint16_t pos_ns;
    uint16_t neg_ns;
    uint32_t gap_ns;
    uint32_t interval_ns;
} psg_group_t;

typedef struct {
    uint8_t group_count;
    psg_group_t groups[PSG_GROUP_MAX];
} psg_sequence_t;

typedef struct {
    uint16_t delay_ms;
    uint16_t interval_ms;
    uint16_t count;
} psg_r_wave_t;

typedef struct {
    float overcurrent_pos;
    float overcurrent_neg;
    float undercurrent_pos;
    float undercurrent_neg;
} psg_thresholds_t;

typedef struct {
    uint16_t voltage_pos;
    uint16_t voltage_neg;
} psg_voltages_t;

typedef struct {
    uint8_t trigger_mode;      /* PRM-319 */
    uint8_t start_source;      /* PRM-323 */
    bool rod_cycle_enabled;    /* PRM-324 */
    uint8_t rod_select;        /* PRM-325 */
    psg_voltages_t voltages;   /* PRM-327 sentinel 0 = unconfigured */
    psg_thresholds_t thresholds; /* PRM-322 sentinel 0.0 = unconfigured */
    bool r_wave_configured;
    psg_r_wave_t r_wave;
    psg_sequence_t sequence;   /* PRM-321 sentinel group_count 0 */
} psg_run_config_t;

void psg_config_defaults(psg_run_config_t *config);

psg_config_result_t psg_config_parse_trigger_mode(uint16_t payload_length, uint8_t value, uint8_t *out);
psg_config_result_t psg_config_parse_start_source(uint16_t payload_length, uint8_t value, uint8_t *out);
psg_config_result_t psg_config_parse_rod_cycle(uint16_t payload_length, uint8_t value,
                                               const psg_sequence_t *sequence, bool *out);
psg_config_result_t psg_config_parse_rod_select(uint16_t payload_length, uint8_t value, uint8_t *out);
psg_config_result_t psg_config_parse_r_wave(const uint8_t *payload, uint16_t payload_length, psg_r_wave_t *out);
psg_config_result_t psg_config_parse_thresholds(const uint8_t *payload, uint16_t payload_length,
                                                psg_thresholds_t *out);
psg_config_result_t psg_config_parse_voltages(const uint8_t *payload, uint16_t payload_length,
                                              psg_voltages_t *out);
psg_config_result_t psg_config_parse_sequence(const uint8_t *payload, uint16_t payload_length,
                                              bool rod_cycle_enabled, psg_sequence_t *out);

#endif /* PSG_RUN_RUN_CONFIG_H */
