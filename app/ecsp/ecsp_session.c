#include "app/ecsp/ecsp_session.h"

void ecsp_session_init(ecsp_session_t *session)
{
    session->connected = false;
    session->command_seen = false;
    session->upload_enabled = false;
}

bool ecsp_session_accept_handshake(ecsp_session_t *session)
{
    bool duplicate = session->connected;

    session->connected = true;
    session->command_seen = true;
    if (duplicate) {
        /* I-012: upload mode returns to its power-up default. */
        session->upload_enabled = false;
    }
    return duplicate;
}

void ecsp_session_note_command(ecsp_session_t *session)
{
    if (session->connected) {
        session->command_seen = true;
    }
}

void ecsp_session_set_upload(ecsp_session_t *session, bool enabled)
{
    session->upload_enabled = enabled;
}

bool ecsp_session_report_allowed(const ecsp_session_t *session)
{
    return session->upload_enabled || (session->connected && session->command_seen);
}
