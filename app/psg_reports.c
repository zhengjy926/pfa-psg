#include "app/psg_reports.h"

#include "app/ecsp/ecsp_frame.h"

void psg_reports_pack_status(const psg_output_status_t *status, uint8_t out[PSG_STATUS_BLOCK_BYTES])
{
    out[0] = status->trigger_mode;
    out[1] = status->channel_status[0];
    out[2] = status->channel_status[1];
    ecsp_write_f32(&out[3], status->channel_current[0]);
    ecsp_write_f32(&out[7], status->channel_current[1]);
    ecsp_write_u16(&out[11], status->completed_triggers);
}
