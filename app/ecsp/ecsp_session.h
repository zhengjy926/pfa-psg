/*
 * ECSP session state (ECSP 4.2, SRS I-012, PRM-120).
 *
 * The slave is "connected" from the handshake reply until reset; it never
 * tracks host-side version-check results.  Upload mode is a session property:
 * off at power-up, switched by 0x0F, forced off by a duplicate handshake.
 */
#ifndef PSG_APP_ECSP_ECSP_SESSION_H
#define PSG_APP_ECSP_ECSP_SESSION_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    bool connected;
    bool command_seen;
    bool upload_enabled;
} ecsp_session_t;

void ecsp_session_init(ecsp_session_t *session);

/* A valid 0x01 request was accepted.  Returns true when this was a duplicate
 * handshake on an established session (caller runs the in-board reset). */
bool ecsp_session_accept_handshake(ecsp_session_t *session);

/* Any valid frame addressed to this board. */
void ecsp_session_note_command(ecsp_session_t *session);

void ecsp_session_set_upload(ecsp_session_t *session, bool enabled);

/* 0x11 gating: upload mode on, or the established-session exception
 * (I-028 / C-0x11-02). */
bool ecsp_session_report_allowed(const ecsp_session_t *session);

#endif /* PSG_APP_ECSP_ECSP_SESSION_H */
