#include "run/run_config.h"

#include <string.h>

#include "shared/psg_bytes.h"

#define PRM_301_GROUPS_MIN 1u
#define PRM_301_GROUPS_MAX 20u
#define PRM_302_GROUP_BYTES 22u
#define PRM_303_BURSTS_MIN 1u
#define PRM_303_BURSTS_MAX 300u
#define PRM_304_WAVES_MIN 1u
#define PRM_304_WAVES_MAX 256u
#define PRM_305_BURST_INTERVAL_MIN 100u
#define PRM_305_BURST_INTERVAL_MAX 10000u
#define PRM_306_GROUP_INTERVAL_MIN 50u
#define PRM_306_GROUP_INTERVAL_MAX 10000u
#define PRM_307_DURATION_MIN 200u
#define PRM_307_DURATION_MAX 5000u
#define PRM_309_GAP_MIN 1000u
#define PRM_309_GAP_MAX 50000u
#define PRM_310_INTERVAL_MIN 1000u
#define PRM_310_INTERVAL_MAX 100000u
#define PRM_311_INTERVAL_MAX 50000u
#define PRM_312_DELAY_MAX 300u
#define PRM_313_INTERVAL_MIN 200u
#define PRM_313_INTERVAL_MAX 2000u
#define PRM_314_COUNT_MIN 1u
#define PRM_314_COUNT_MAX 300u
#define PRM_401_VOLTAGE_MAX 4000u
#define PRM_404_CURRENT_MAX 90.0f
#define PSG_ROD_PAIR_STRIDE 6u

void psg_config_defaults(psg_run_config_t *config)
{
    (void)memset(config, 0, sizeof(*config));
    config->trigger_mode = 0u;          /* PRM-319 direct trigger */
    config->start_source = 0u;          /* PRM-323 protocol */
    config->rod_cycle_enabled = false;  /* PRM-324 */
    config->rod_select = 0x3Fu;         /* PRM-325 all rods */
    config->r_wave_configured = false;  /* PRM-320 */
    config->sequence.group_count = 0u;  /* PRM-321 unconfigured */
}

static bool duration_valid(uint16_t value)
{
    return (value == 0u) || ((value >= PRM_307_DURATION_MIN) && (value <= PRM_307_DURATION_MAX));
}

psg_config_result_t psg_config_parse_trigger_mode(uint16_t payload_length, uint8_t value, uint8_t *out)
{
    if (payload_length != 1u) {
        return PSG_CONFIG_BAD_LENGTH;
    }
    if (value > 1u) {
        return PSG_CONFIG_BAD_PARAMETER;
    }
    *out = value;
    return PSG_CONFIG_OK;
}

psg_config_result_t psg_config_parse_start_source(uint16_t payload_length, uint8_t value, uint8_t *out)
{
    if (payload_length != 1u) {
        return PSG_CONFIG_BAD_LENGTH;
    }
    if (value > 1u) {
        return PSG_CONFIG_BAD_PARAMETER;
    }
    *out = value;
    return PSG_CONFIG_OK;
}

psg_config_result_t psg_config_parse_rod_cycle(uint16_t payload_length, uint8_t value,
                                               const psg_sequence_t *sequence, bool *out)
{
    uint8_t index;

    if (payload_length != 1u) {
        return PSG_CONFIG_BAD_LENGTH;
    }
    if (value > 1u) {
        return PSG_CONFIG_BAD_PARAMETER;
    }
    if (value == 1u) {
        for (index = 0u; index < sequence->group_count; index++) {
            if ((sequence->groups[index].burst_count % PSG_ROD_PAIR_STRIDE) != 0u) {
                return PSG_CONFIG_BAD_PARAMETER;
            }
        }
    }
    *out = (value == 1u);
    return PSG_CONFIG_OK;
}

psg_config_result_t psg_config_parse_rod_select(uint16_t payload_length, uint8_t value, uint8_t *out)
{
    static const uint8_t legal[] = {0x3Fu, 0x03u, 0x0Cu, 0x30u, 0x06u, 0x21u, 0x18u};
    uint8_t index;
    bool found = false;

    if (payload_length != 1u) {
        return PSG_CONFIG_BAD_LENGTH;
    }
    for (index = 0u; index < (uint8_t)(sizeof(legal) / sizeof(legal[0])); index++) {
        if (legal[index] == value) {
            found = true;
            break;
        }
    }
    if (!found) {
        return PSG_CONFIG_BAD_PARAMETER;
    }
    *out = value;
    return PSG_CONFIG_OK;
}

psg_config_result_t psg_config_parse_r_wave(const uint8_t *payload, uint16_t payload_length, psg_r_wave_t *out)
{
    uint16_t delay_ms;
    uint16_t interval_ms;
    uint16_t count;

    if ((payload == NULL) || (payload_length != 6u)) {
        return PSG_CONFIG_BAD_LENGTH;
    }
    delay_ms = ecsp_read_u16(&payload[0]);
    interval_ms = ecsp_read_u16(&payload[2]);
    count = ecsp_read_u16(&payload[4]);
    if (delay_ms > PRM_312_DELAY_MAX) {
        return PSG_CONFIG_BAD_PARAMETER;
    }
    if ((interval_ms < PRM_313_INTERVAL_MIN) || (interval_ms > PRM_313_INTERVAL_MAX)) {
        return PSG_CONFIG_BAD_PARAMETER;
    }
    if ((count < PRM_314_COUNT_MIN) || (count > PRM_314_COUNT_MAX)) {
        return PSG_CONFIG_BAD_PARAMETER;
    }
    out->delay_ms = delay_ms;
    out->interval_ms = interval_ms;
    out->count = count;
    return PSG_CONFIG_OK;
}

psg_config_result_t psg_config_parse_thresholds(const uint8_t *payload, uint16_t payload_length,
                                                psg_thresholds_t *out)
{
    float over_pos;
    float over_neg;
    float under_pos;
    float under_neg;

    if ((payload == NULL) || (payload_length != 16u)) {
        return PSG_CONFIG_BAD_LENGTH;
    }
    over_pos = ecsp_read_f32(&payload[0]);
    over_neg = ecsp_read_f32(&payload[4]);
    under_pos = ecsp_read_f32(&payload[8]);
    under_neg = ecsp_read_f32(&payload[12]);

    /* NaN fails both comparisons and +Inf fails the upper bound, so the
     * finite-value requirement (C-0x38-02) is covered without libm. */
    if (!((over_pos > 0.0f) && (over_pos <= PRM_404_CURRENT_MAX))) {
        return PSG_CONFIG_BAD_PARAMETER;
    }
    if (!((over_neg > 0.0f) && (over_neg <= PRM_404_CURRENT_MAX))) {
        return PSG_CONFIG_BAD_PARAMETER;
    }
    if (!((under_pos > 0.0f) && (under_pos < over_pos))) {
        return PSG_CONFIG_BAD_PARAMETER;
    }
    if (!((under_neg > 0.0f) && (under_neg < over_neg))) {
        return PSG_CONFIG_BAD_PARAMETER;
    }

    out->overcurrent_pos = over_pos;
    out->overcurrent_neg = over_neg;
    out->undercurrent_pos = under_pos;
    out->undercurrent_neg = under_neg;
    return PSG_CONFIG_OK;
}

psg_config_result_t psg_config_parse_voltages(const uint8_t *payload, uint16_t payload_length,
                                              psg_voltages_t *out)
{
    uint16_t pos;
    uint16_t neg;

    if ((payload == NULL) || (payload_length != 4u)) {
        return PSG_CONFIG_BAD_LENGTH;
    }
    pos = ecsp_read_u16(&payload[0]);
    neg = ecsp_read_u16(&payload[2]);
    if ((pos == 0u) || (pos > PRM_401_VOLTAGE_MAX) || (neg == 0u) || (neg > PRM_401_VOLTAGE_MAX)) {
        return PSG_CONFIG_BAD_PARAMETER;
    }
    out->voltage_pos = pos;
    out->voltage_neg = neg;
    return PSG_CONFIG_OK;
}

static psg_config_result_t parse_group(const uint8_t *block, uint8_t index, bool rod_cycle_enabled,
                                       psg_group_t *group)
{
    group->group_count_field = block[0];
    group->group_index = block[1];
    group->group_interval_ms = ecsp_read_u16(&block[2]);
    group->burst_count = ecsp_read_u16(&block[4]);
    group->wave_count = ecsp_read_u16(&block[6]);
    group->burst_interval_10us = ecsp_read_u16(&block[8]);
    group->pos_ns = ecsp_read_u16(&block[10]);
    group->neg_ns = ecsp_read_u16(&block[12]);
    group->gap_ns = ecsp_read_u32(&block[14]);
    group->interval_ns = ecsp_read_u32(&block[18]);

    if (group->group_index != (uint8_t)(index + 1u)) {
        return PSG_CONFIG_BAD_PARAMETER;
    }
    if ((group->group_interval_ms < PRM_306_GROUP_INTERVAL_MIN) ||
        (group->group_interval_ms > PRM_306_GROUP_INTERVAL_MAX)) {
        return PSG_CONFIG_BAD_PARAMETER;
    }
    if ((group->burst_count < PRM_303_BURSTS_MIN) || (group->burst_count > PRM_303_BURSTS_MAX)) {
        return PSG_CONFIG_BAD_PARAMETER;
    }
    if (rod_cycle_enabled && ((group->burst_count % PSG_ROD_PAIR_STRIDE) != 0u)) {
        return PSG_CONFIG_BAD_PARAMETER;
    }
    if ((group->wave_count < PRM_304_WAVES_MIN) || (group->wave_count > PRM_304_WAVES_MAX)) {
        return PSG_CONFIG_BAD_PARAMETER;
    }
    if ((group->burst_interval_10us < PRM_305_BURST_INTERVAL_MIN) ||
        (group->burst_interval_10us > PRM_305_BURST_INTERVAL_MAX)) {
        return PSG_CONFIG_BAD_PARAMETER;
    }
    if ((!duration_valid(group->pos_ns)) || (!duration_valid(group->neg_ns))) {
        return PSG_CONFIG_BAD_PARAMETER;
    }
    if ((group->pos_ns == 0u) && (group->neg_ns == 0u)) {
        return PSG_CONFIG_BAD_PARAMETER;
    }

    if ((group->pos_ns != 0u) && (group->neg_ns != 0u)) {
        /* Bipolar (F-019). */
        if ((group->gap_ns < PRM_309_GAP_MIN) || (group->gap_ns > PRM_309_GAP_MAX)) {
            return PSG_CONFIG_BAD_PARAMETER;
        }
        if ((group->interval_ns < PRM_310_INTERVAL_MIN) || (group->interval_ns > PRM_311_INTERVAL_MAX)) {
            return PSG_CONFIG_BAD_PARAMETER;
        }
    } else {
        /* Unipolar: no inter-polarity gap and the wider interval range. */
        if (group->gap_ns != 0u) {
            return PSG_CONFIG_BAD_PARAMETER;
        }
        if ((group->interval_ns < PRM_310_INTERVAL_MIN) || (group->interval_ns > PRM_310_INTERVAL_MAX)) {
            return PSG_CONFIG_BAD_PARAMETER;
        }
    }
    return PSG_CONFIG_OK;
}

psg_config_result_t psg_config_parse_sequence(const uint8_t *payload, uint16_t payload_length,
                                              bool rod_cycle_enabled, psg_sequence_t *out)
{
    uint16_t blocks;
    uint16_t offset;
    uint8_t index;
    uint16_t interval_ref = 0u;

    if ((payload == NULL) || (payload_length == 0u) || ((payload_length % PRM_302_GROUP_BYTES) != 0u)) {
        return PSG_CONFIG_BAD_LENGTH;
    }
    blocks = (uint16_t)(payload_length / PRM_302_GROUP_BYTES);
    if ((blocks < PRM_301_GROUPS_MIN) || (blocks > PRM_301_GROUPS_MAX)) {
        return PSG_CONFIG_BAD_LENGTH;
    }
    if ((payload[0] < PRM_301_GROUPS_MIN) || (payload[0] > PRM_301_GROUPS_MAX)) {
        return PSG_CONFIG_BAD_PARAMETER;
    }
    if ((uint16_t)payload[0] != blocks) {
        return PSG_CONFIG_BAD_PARAMETER;
    }

    out->group_count = (uint8_t)blocks;
    offset = 0u;
    for (index = 0u; index < (uint8_t)blocks; index++) {
        psg_config_result_t result = parse_group(&payload[offset], index, rod_cycle_enabled,
                                                 &out->groups[index]);
        if (result != PSG_CONFIG_OK) {
            return result;
        }
        if ((index == 0u) && (out->groups[0].group_count_field != (uint8_t)blocks)) {
            return PSG_CONFIG_BAD_PARAMETER;
        }
        if (index == 0u) {
            interval_ref = out->groups[0].group_interval_ms;
        } else if (out->groups[index].group_interval_ms != interval_ref) {
            return PSG_CONFIG_BAD_PARAMETER;
        }
        offset = (uint16_t)(offset + PRM_302_GROUP_BYTES);
    }
    return PSG_CONFIG_OK;
}
