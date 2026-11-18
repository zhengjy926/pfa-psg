#include "app/psg_ecsp.h"

#include <string.h>

#include "app/app_config.h"
#include "app/ecsp/ecsp_codes.h"
#include "app/psg_reports.h"

#define PSG_GROUP_STRIDE PSG_GROUP_ENCODED_BYTES

static uint8_t verdict_code(psg_verdict_t verdict)
{
    switch (verdict) {
    case PSG_VERDICT_ACCEPT:
        return ECSP_RSP_SUCCESS;
    case PSG_VERDICT_BUSY:
        return ECSP_RSP_BUSY;
    case PSG_VERDICT_MODE:
        return ECSP_RSP_MODE;
    case PSG_VERDICT_SAFETY:
        return ECSP_RSP_SAFETY;
    case PSG_VERDICT_LOCKED:
        return ECSP_RSP_LOCKED;
    case PSG_VERDICT_PRECONDITION:
    default:
        return ECSP_RSP_PRECONDITION;
    }
}

static uint8_t config_code(psg_config_result_t result)
{
    if (result == PSG_CONFIG_BAD_LENGTH) {
        return ECSP_RSP_BAD_LENGTH;
    }
    if (result == PSG_CONFIG_BAD_PARAMETER) {
        return ECSP_RSP_BAD_PARAMETER;
    }
    return ECSP_RSP_SUCCESS;
}

static void reply_small(psg_ecsp_t *svc, uint8_t command, uint8_t code, const uint8_t *payload,
                        uint16_t payload_length)
{
    uint8_t buffer[ECSP_TX_SMALL_PAYLOAD_BYTES];

    if ((uint32_t)payload_length + 1u > sizeof(buffer)) {
        return;
    }
    buffer[0] = code;
    if (payload_length != 0u) {
        (void)memcpy(&buffer[1], payload, (size_t)payload_length);
    }
    (void)ecsp_tx_submit_reply(&svc->tx, command, buffer, (uint16_t)(payload_length + 1u));
}

static void reply_code(psg_ecsp_t *svc, uint8_t command, uint8_t code)
{
    reply_small(svc, command, code, NULL, 0u);
}

static bool command_supported(uint8_t command)
{
    switch (command) {
    case ECSP_CMD_HANDSHAKE:
    case ECSP_CMD_GET_SW_VERSION:
    case ECSP_CMD_GET_HW_VERSION:
    case ECSP_CMD_GET_SERIAL:
    case ECSP_CMD_RESET:
    case ECSP_CMD_SET_UPLOAD_MODE:
    case ECSP_CMD_GET_FAULT:
    case 0x30u: /* start/stop */
    case 0x31u: /* output status */
    case 0x32u: /* trigger mode */
    case 0x33u:
    case 0x34u: /* sequence */
    case 0x35u:
    case 0x36u: /* R-wave */
    case 0x37u:
    case 0x38u: /* thresholds */
    case 0x39u:
    case 0x3Au: /* voltages */
    case 0x3Bu:
    case 0x3Du: /* clear current anomaly */
    case 0x3Eu: /* start source */
    case 0x3Fu:
    case 0x40u: /* rod cycle */
    case 0x41u:
    case 0x42u: /* rod select */
    case 0x43u:
        return true;
    default:
        /* 0x03/0x05 identity writes arrive with the identity ticket. */
        return false;
    }
}

static void handle_handshake(psg_ecsp_t *svc, uint16_t payload_length, uint64_t now_us)
{
    bool duplicate;

    if (payload_length != 0u) {
        reply_code(svc, ECSP_CMD_HANDSHAKE, ECSP_RSP_BAD_LENGTH);
        return;
    }

    duplicate = ecsp_session_accept_handshake(&svc->session);
    if (duplicate) {
        psg_run_reset_after_handshake(svc->config.run);
    }
    /* ECSP 4.3: every accepted handshake clears the transaction cache. */
    dedup_clear(&svc->dedup);
    if (duplicate) {
        ecsp_tx_flush(&svc->tx);
    }
    ecsp_tx_set_connected(&svc->tx, true, now_us);
    reply_code(svc, ECSP_CMD_HANDSHAKE, ECSP_RSP_SUCCESS);
}

static void handle_start_stop(psg_ecsp_t *svc, const ecsp_frame_t *frame)
{
    uint8_t sequence;
    uint8_t control;
    uint8_t code;
    psg_verdict_t verdict;
    dedup_entry_t hit;

    if (frame->payload_length != 2u) {
        reply_code(svc, 0x30u, ECSP_RSP_BAD_LENGTH);
        return;
    }
    sequence = frame->payload[0];
    control = frame->payload[1];
    if (control > 1u) {
        reply_code(svc, 0x30u, ECSP_RSP_BAD_PARAMETER);
        return;
    }

    if (dedup_lookup(&svc->dedup, 0x30u, sequence, &hit)) {
        reply_small(svc, 0x30u, hit.code, hit.payload, hit.payload_length);
        return;
    }

    if (control == 1u) {
        verdict = psg_run_start_verdict(svc->config.run, 0u);
        if (verdict == PSG_VERDICT_ACCEPT) {
            verdict = psg_run_start(svc->config.run);
        }
    } else {
        verdict = psg_run_stop_verdict(svc->config.run, 0u);
        if (verdict == PSG_VERDICT_ACCEPT) {
            psg_run_stop(svc->config.run);
        }
    }

    code = verdict_code(verdict);
    reply_code(svc, 0x30u, code);
    dedup_store(&svc->dedup, 0x30u, sequence, code, NULL, 0u);
}

static void handle_clear(psg_ecsp_t *svc, uint16_t payload_length, uint64_t now_us)
{
    psg_verdict_t verdict;

    if (payload_length != 0u) {
        reply_code(svc, 0x3Du, ECSP_RSP_BAD_LENGTH);
        return;
    }
    if (svc->clear_pending) {
        reply_code(svc, 0x3Du, ECSP_RSP_BUSY);
        return;
    }
    verdict = psg_run_clear_verdict(svc->config.run);
    if (verdict != PSG_VERDICT_ACCEPT) {
        reply_code(svc, 0x3Du, verdict_code(verdict));
        return;
    }
    psg_run_clear_begin(svc->config.run, now_us);
    svc->clear_pending = true;
}

static void handle_trigger_mode(psg_ecsp_t *svc, const ecsp_frame_t *frame)
{
    uint8_t value = 0u;
    psg_config_result_t parsed;
    psg_verdict_t verdict;

    if (frame->payload_length != 1u) {
        reply_code(svc, 0x32u, ECSP_RSP_BAD_LENGTH);
        return;
    }
    parsed = psg_config_parse_trigger_mode(frame->payload_length, frame->payload[0], &value);
    if (parsed != PSG_CONFIG_OK) {
        reply_code(svc, 0x32u, config_code(parsed));
        return;
    }
    verdict = psg_run_config_change_verdict(svc->config.run);
    if (verdict != PSG_VERDICT_ACCEPT) {
        reply_code(svc, 0x32u, verdict_code(verdict));
        return;
    }
    psg_run_commit_trigger_mode(svc->config.run, value);
    reply_code(svc, 0x32u, ECSP_RSP_SUCCESS);
}

static void handle_sequence(psg_ecsp_t *svc, const ecsp_frame_t *frame)
{
    psg_config_result_t parsed;
    psg_verdict_t verdict;

    /* Payload length is checked inside the parser so an oversized/garbled
     * block returns 0x02 before any state verdict. */
    parsed = psg_config_parse_sequence(frame->payload, frame->payload_length,
                                       svc->config.run->config.rod_cycle_enabled,
                                       &svc->sequence_candidate);
    if (parsed != PSG_CONFIG_OK) {
        reply_code(svc, 0x34u, config_code(parsed));
        return;
    }
    verdict = psg_run_config_change_verdict(svc->config.run);
    if (verdict != PSG_VERDICT_ACCEPT) {
        reply_code(svc, 0x34u, verdict_code(verdict));
        return;
    }
    psg_run_commit_sequence(svc->config.run, &svc->sequence_candidate);
    reply_code(svc, 0x34u, ECSP_RSP_SUCCESS);
}

static size_t build_sequence_reply(psg_ecsp_t *svc)
{
    const psg_sequence_t *sequence = &svc->config.run->config.sequence;
    size_t header;
    size_t frame_length;
    uint8_t index;

    header = ecsp_frame_begin(svc->large_response, sizeof(svc->large_response), svc->config.product_address,
                              svc->config.board_address, 0x35u,
                              (uint16_t)(1u + ((uint16_t)sequence->group_count * PSG_GROUP_STRIDE)));
    if (header == 0u) {
        return 0u;
    }
    svc->large_response[6] = ECSP_RSP_SUCCESS;
    for (index = 0u; index < sequence->group_count; index++) {
        const psg_group_t *group = &sequence->groups[index];
        uint8_t *block = &svc->large_response[7u + ((size_t)index * PSG_GROUP_STRIDE)];

        block[0] = group->group_count_field;
        block[1] = group->group_index;
        ecsp_write_u16(&block[2], group->group_interval_ms);
        ecsp_write_u16(&block[4], group->burst_count);
        ecsp_write_u16(&block[6], group->wave_count);
        ecsp_write_u16(&block[8], group->burst_interval_10us);
        ecsp_write_u16(&block[10], group->pos_ns);
        ecsp_write_u16(&block[12], group->neg_ns);
        ecsp_write_u32(&block[14], group->gap_ns);
        ecsp_write_u32(&block[18], group->interval_ns);
    }
    frame_length = header;
    ecsp_frame_finish(svc->large_response, frame_length);
    return frame_length;
}

static void handle_frame(psg_ecsp_t *svc, const ecsp_frame_t *frame, uint64_t now_us)
{
    psg_run_config_t *run_config;

    if ((frame->product != svc->config.product_address) || (frame->board != svc->config.board_address)) {
        return; /* ECSP 5.3: address mismatch is discarded with no action. */
    }
    if (svc->reset_pending) {
        return;
    }
    ecsp_session_note_command(&svc->session);
    svc->frames_handled++;

    if (!command_supported(frame->command)) {
        reply_code(svc, frame->command, ECSP_RSP_UNSUPPORTED);
        return;
    }
    if ((!svc->session.connected) && (frame->command != ECSP_CMD_HANDSHAKE)) {
        reply_code(svc, frame->command, ECSP_RSP_NOT_HANDSHAKEN);
        return;
    }

    run_config = &svc->config.run->config;

    switch (frame->command) {
    case ECSP_CMD_HANDSHAKE:
        handle_handshake(svc, frame->payload_length, now_us);
        break;

    case ECSP_CMD_GET_SW_VERSION:
        if (frame->payload_length != 0u) {
            reply_code(svc, frame->command, ECSP_RSP_BAD_LENGTH);
        } else {
            reply_small(svc, frame->command, ECSP_RSP_SUCCESS, identity_software_version(), PSG_VERSION_BYTES);
        }
        break;

    case ECSP_CMD_GET_HW_VERSION:
        if (frame->payload_length != 0u) {
            reply_code(svc, frame->command, ECSP_RSP_BAD_LENGTH);
        } else {
            reply_small(svc, frame->command, ECSP_RSP_SUCCESS,
                        identity_hardware_version(svc->config.identity), PSG_VERSION_BYTES);
        }
        break;

    case ECSP_CMD_GET_SERIAL:
        if (frame->payload_length != 0u) {
            reply_code(svc, frame->command, ECSP_RSP_BAD_LENGTH);
        } else {
            reply_small(svc, frame->command, ECSP_RSP_SUCCESS,
                        identity_serial_number(svc->config.identity), PSG_SERIAL_NUMBER_BYTES);
        }
        break;

    case ECSP_CMD_RESET: {
        dedup_entry_t hit;

        if (frame->payload_length != 1u) {
            reply_code(svc, frame->command, ECSP_RSP_BAD_LENGTH);
            break;
        }
        if (dedup_lookup(&svc->dedup, ECSP_CMD_RESET, frame->payload[0], &hit)) {
            reply_small(svc, frame->command, hit.code, hit.payload, hit.payload_length);
            break;
        }
        reply_code(svc, frame->command, ECSP_RSP_SUCCESS);
        ecsp_tx_request_reset_after_reply(&svc->tx);
        dedup_store(&svc->dedup, ECSP_CMD_RESET, frame->payload[0], ECSP_RSP_SUCCESS, NULL, 0u);
        svc->reset_pending = true;
        break;
    }

    case ECSP_CMD_SET_UPLOAD_MODE: {
        psg_verdict_t verdict;

        if (frame->payload_length != 1u) {
            reply_code(svc, frame->command, ECSP_RSP_BAD_LENGTH);
            break;
        }
        if (frame->payload[0] > 1u) {
            reply_code(svc, frame->command, ECSP_RSP_BAD_PARAMETER);
            break;
        }
        verdict = psg_run_config_change_verdict(svc->config.run);
        if (verdict != PSG_VERDICT_ACCEPT) {
            reply_code(svc, frame->command, verdict_code(verdict));
            break;
        }
        ecsp_session_set_upload(&svc->session, frame->payload[0] == 1u);
        reply_code(svc, frame->command, ECSP_RSP_SUCCESS);
        break;
    }

    case ECSP_CMD_GET_FAULT:
        if (frame->payload_length != 0u) {
            reply_code(svc, frame->command, ECSP_RSP_BAD_LENGTH);
        } else {
            uint8_t word = psg_run_fault_word(svc->config.run);
            reply_small(svc, frame->command, ECSP_RSP_SUCCESS, &word, 1u);
        }
        break;

    case 0x30u:
        handle_start_stop(svc, frame);
        break;

    case 0x31u: {
        uint8_t block[PSG_STATUS_BLOCK_BYTES];

        if (frame->payload_length != 0u) {
            reply_code(svc, frame->command, ECSP_RSP_BAD_LENGTH);
            break;
        }
        psg_reports_pack_status(psg_run_status(svc->config.run), block);
        reply_small(svc, frame->command, ECSP_RSP_SUCCESS, block, PSG_STATUS_BLOCK_BYTES);
        break;
    }

    case 0x32u:
        handle_trigger_mode(svc, frame);
        break;

    case 0x33u:
        if (frame->payload_length != 0u) {
            reply_code(svc, frame->command, ECSP_RSP_BAD_LENGTH);
        } else {
            uint8_t mode = svc->config.run->config.trigger_mode;
            reply_small(svc, frame->command, ECSP_RSP_SUCCESS, &mode, 1u);
        }
        break;

    case 0x34u:
        handle_sequence(svc, frame);
        break;

    case 0x35u: {
        size_t length;

        if (frame->payload_length != 0u) {
            reply_code(svc, frame->command, ECSP_RSP_BAD_LENGTH);
            break;
        }
        length = build_sequence_reply(svc);
        if (length == 0u) {
            reply_code(svc, frame->command, ECSP_RSP_EXECUTION_FAILED);
            break;
        }
        (void)ecsp_tx_submit_large_reply(&svc->tx, svc->large_response, (uint16_t)length);
        break;
    }

    case 0x36u: {
        psg_r_wave_t r_wave;
        psg_config_result_t parsed = psg_config_parse_r_wave(frame->payload, frame->payload_length, &r_wave);
        psg_verdict_t verdict;

        if (parsed != PSG_CONFIG_OK) {
            reply_code(svc, frame->command, config_code(parsed));
            break;
        }
        verdict = psg_run_config_change_verdict(svc->config.run);
        if (verdict != PSG_VERDICT_ACCEPT) {
            reply_code(svc, frame->command, verdict_code(verdict));
            break;
        }
        psg_run_commit_r_wave(svc->config.run, &r_wave);
        reply_code(svc, frame->command, ECSP_RSP_SUCCESS);
        break;
    }

    case 0x37u:
        if (frame->payload_length != 0u) {
            reply_code(svc, frame->command, ECSP_RSP_BAD_LENGTH);
        } else {
            uint8_t block[6];
            ecsp_write_u16(&block[0], svc->config.run->config.r_wave.delay_ms);
            ecsp_write_u16(&block[2], svc->config.run->config.r_wave.interval_ms);
            ecsp_write_u16(&block[4], svc->config.run->config.r_wave.count);
            reply_small(svc, frame->command, ECSP_RSP_SUCCESS, block, sizeof(block));
        }
        break;

    case 0x38u: {
        psg_thresholds_t thresholds;
        psg_config_result_t parsed = psg_config_parse_thresholds(frame->payload, frame->payload_length,
                                                                 &thresholds);
        psg_verdict_t verdict;

        if (parsed != PSG_CONFIG_OK) {
            reply_code(svc, frame->command, config_code(parsed));
            break;
        }
        verdict = psg_run_config_change_verdict(svc->config.run);
        if (verdict != PSG_VERDICT_ACCEPT) {
            reply_code(svc, frame->command, verdict_code(verdict));
            break;
        }
        psg_run_commit_thresholds(svc->config.run, &thresholds);
        reply_code(svc, frame->command, ECSP_RSP_SUCCESS);
        break;
    }

    case 0x39u:
        if (frame->payload_length != 0u) {
            reply_code(svc, frame->command, ECSP_RSP_BAD_LENGTH);
        } else {
            uint8_t block[16];
            ecsp_write_f32(&block[0], svc->config.run->config.thresholds.overcurrent_pos);
            ecsp_write_f32(&block[4], svc->config.run->config.thresholds.overcurrent_neg);
            ecsp_write_f32(&block[8], svc->config.run->config.thresholds.undercurrent_pos);
            ecsp_write_f32(&block[12], svc->config.run->config.thresholds.undercurrent_neg);
            reply_small(svc, frame->command, ECSP_RSP_SUCCESS, block, sizeof(block));
        }
        break;

    case 0x3Au: {
        psg_voltages_t voltages;
        psg_config_result_t parsed = psg_config_parse_voltages(frame->payload, frame->payload_length, &voltages);
        psg_verdict_t verdict;

        if (parsed != PSG_CONFIG_OK) {
            reply_code(svc, frame->command, config_code(parsed));
            break;
        }
        verdict = psg_run_config_change_verdict(svc->config.run);
        if (verdict != PSG_VERDICT_ACCEPT) {
            reply_code(svc, frame->command, verdict_code(verdict));
            break;
        }
        psg_run_commit_voltages(svc->config.run, &voltages);
        reply_code(svc, frame->command, ECSP_RSP_SUCCESS);
        break;
    }

    case 0x3Bu:
        if (frame->payload_length != 0u) {
            reply_code(svc, frame->command, ECSP_RSP_BAD_LENGTH);
        } else {
            uint8_t block[4];
            ecsp_write_u16(&block[0], svc->config.run->config.voltages.voltage_pos);
            ecsp_write_u16(&block[2], svc->config.run->config.voltages.voltage_neg);
            reply_small(svc, frame->command, ECSP_RSP_SUCCESS, block, sizeof(block));
        }
        break;

    case 0x3Du:
        handle_clear(svc, frame->payload_length, now_us);
        break;

    case 0x3Eu: {
        uint8_t value = 0u;
        psg_config_result_t parsed;
        psg_verdict_t verdict;

        if (frame->payload_length != 1u) {
            reply_code(svc, frame->command, ECSP_RSP_BAD_LENGTH);
            break;
        }
        parsed = psg_config_parse_start_source(frame->payload_length, frame->payload[0], &value);
        if (parsed != PSG_CONFIG_OK) {
            reply_code(svc, frame->command, config_code(parsed));
            break;
        }
        verdict = psg_run_config_change_verdict(svc->config.run);
        if (verdict != PSG_VERDICT_ACCEPT) {
            reply_code(svc, frame->command, verdict_code(verdict));
            break;
        }
        psg_run_commit_start_source(svc->config.run, value);
        reply_code(svc, frame->command, ECSP_RSP_SUCCESS);
        break;
    }

    case 0x3Fu:
        if (frame->payload_length != 0u) {
            reply_code(svc, frame->command, ECSP_RSP_BAD_LENGTH);
        } else {
            uint8_t source = svc->config.run->config.start_source;
            reply_small(svc, frame->command, ECSP_RSP_SUCCESS, &source, 1u);
        }
        break;

    case 0x40u: {
        bool enabled = false;
        psg_config_result_t parsed;
        psg_verdict_t verdict;

        if (frame->payload_length != 1u) {
            reply_code(svc, frame->command, ECSP_RSP_BAD_LENGTH);
            break;
        }
        parsed = psg_config_parse_rod_cycle(frame->payload_length, frame->payload[0],
                                            &run_config->sequence, &enabled);
        if (parsed != PSG_CONFIG_OK) {
            reply_code(svc, frame->command, config_code(parsed));
            break;
        }
        verdict = psg_run_config_change_verdict(svc->config.run);
        if (verdict != PSG_VERDICT_ACCEPT) {
            reply_code(svc, frame->command, verdict_code(verdict));
            break;
        }
        psg_run_commit_rod_cycle(svc->config.run, enabled);
        reply_code(svc, frame->command, ECSP_RSP_SUCCESS);
        break;
    }

    case 0x41u:
        if (frame->payload_length != 0u) {
            reply_code(svc, frame->command, ECSP_RSP_BAD_LENGTH);
        } else {
            uint8_t enabled = svc->config.run->config.rod_cycle_enabled ? 1u : 0u;
            reply_small(svc, frame->command, ECSP_RSP_SUCCESS, &enabled, 1u);
        }
        break;

    case 0x42u: {
        uint8_t value = 0u;
        psg_config_result_t parsed;
        psg_verdict_t verdict;

        if (frame->payload_length != 1u) {
            reply_code(svc, frame->command, ECSP_RSP_BAD_LENGTH);
            break;
        }
        parsed = psg_config_parse_rod_select(frame->payload_length, frame->payload[0], &value);
        if (parsed != PSG_CONFIG_OK) {
            reply_code(svc, frame->command, config_code(parsed));
            break;
        }
        verdict = psg_run_config_change_verdict(svc->config.run);
        if (verdict != PSG_VERDICT_ACCEPT) {
            reply_code(svc, frame->command, verdict_code(verdict));
            break;
        }
        psg_run_commit_rod_select(svc->config.run, value);
        reply_code(svc, frame->command, ECSP_RSP_SUCCESS);
        break;
    }

    case 0x43u:
        if (frame->payload_length != 0u) {
            reply_code(svc, frame->command, ECSP_RSP_BAD_LENGTH);
        } else {
            uint8_t value = svc->config.run->config.rod_select;
            reply_small(svc, frame->command, ECSP_RSP_SUCCESS, &value, 1u);
        }
        break;

    default:
        reply_code(svc, frame->command, ECSP_RSP_UNSUPPORTED);
        break;
    }
}

void psg_ecsp_init(psg_ecsp_t *svc, const psg_ecsp_config_t *config, uint64_t now_us)
{
    (void)memset(svc, 0, sizeof(*svc));
    svc->config = *config;
    ecsp_rx_init(&svc->rx);
    ecsp_tx_init(&svc->tx, config->uart, config->product_address, config->board_address, now_us);
    ecsp_session_init(&svc->session);
    dedup_init(&svc->dedup);
    svc->last_fault_word = psg_run_fault_word(config->run);
    svc->reset_pending = false;
    svc->clear_pending = false;
}

void psg_ecsp_feed(psg_ecsp_t *svc, const uart_rx_byte_t *bytes, size_t count, uint32_t gaps)
{
    if (gaps != 0u) {
        ecsp_rx_note_gap(&svc->rx);
    }
    (void)ecsp_rx_feed(&svc->rx, bytes, count);
}

void psg_ecsp_step(psg_ecsp_t *svc, uint64_t now_us)
{
    uint8_t word;

    if (svc->clear_pending && psg_run_clear_due(svc->config.run, now_us)) {
        bool cleared = psg_run_clear_finish(svc->config.run);

        svc->clear_pending = false;
        reply_code(svc, 0x3Du, cleared ? ECSP_RSP_SUCCESS : ECSP_RSP_EXECUTION_FAILED);
        if ((!cleared) && (svc->session.upload_enabled)) {
            ecsp_tx_submit_fault(&svc->tx, psg_run_fault_word(svc->config.run));
        }
    }

    while (ecsp_rx_poll(&svc->rx, (uint32_t)now_us, &svc->frame)) {
        handle_frame(svc, &svc->frame, now_us);
        if (svc->reset_pending) {
            break;
        }
    }

    {
        psg_run_result_t result;

        if (psg_run_take_result(svc->config.run, &result)) {
            if (svc->session.upload_enabled) {
                uint8_t block[PSG_STATUS_BLOCK_BYTES];
                psg_reports_pack_status(&result.status, block);
                (void)ecsp_tx_submit_event(&svc->tx, 0x3Cu, block, PSG_STATUS_BLOCK_BYTES);
            }
        }
    }

    word = psg_run_fault_word(svc->config.run);
    if (psg_fault_has_new_bits(svc->last_fault_word, word) && ecsp_session_report_allowed(&svc->session)) {
        ecsp_tx_submit_fault(&svc->tx, word);
    }
    svc->last_fault_word = word;

    ecsp_tx_pump(&svc->tx, now_us);

    if (ecsp_tx_take_reset(&svc->tx)) {
        svc->reset_pending = true;
        psg_board_request_reset(&svc->config.board);
    }
}
