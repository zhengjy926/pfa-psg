/*
 * Compile-time configuration for PSG v3 (SRS N-001, architecture section 2).
 *
 * Shared values live in shared/psg_config.h so lower layers can include them
 * without depending on app/.  This header keeps the application registry that
 * the architecture names, plus the app-only knobs.
 */
#ifndef PSG_APP_CONFIG_H
#define PSG_APP_CONFIG_H

#include "shared/psg_config.h"

/* Task stack sizes in bytes; configCHECK_FOR_STACK_OVERFLOW=2 guards them. */
#define PSG_STACK_MONITOR_BYTES 512u
#define PSG_STACK_PULSE_BYTES 768u
#define PSG_STACK_SETTLEMENT_BYTES 512u
#define PSG_STACK_PROTOCOL_BYTES 1024u
#define PSG_STACK_UART_BYTES 512u
#define PSG_STACK_IDENTITY_BYTES 512u

#endif /* PSG_APP_CONFIG_H */
