/*
 * Logging facade (N-005).  Release builds keep ERROR and WARN; debug builds
 * open INFO and DEBUG.  The backend is supplied by the board (SEGGER RTT on
 * the target, a capture buffer on the host) and never shares the protocol
 * port.
 */
#ifndef PSG_UTILS_PSG_LOG_H
#define PSG_UTILS_PSG_LOG_H

#include "shared/psg_config.h"

typedef enum {
    PSG_LOG_ERROR = 0,
    PSG_LOG_WARN = 1,
    PSG_LOG_INFO = 2,
    PSG_LOG_DEBUG = 3
} psg_log_level_t;

#define PSG_LOG_LEVEL_INFO 2
#define PSG_LOG_LEVEL_DEBUG 3

void psg_log_write(psg_log_level_t level, const char *message);

/* Board-provided sink (RTT on the target).  The default implementation in
 * psg_log.c is weak and drops messages. */
void psg_log_backend_write(psg_log_level_t level, const char *message);

#if PSG_LOG_LEVEL >= PSG_LOG_LEVEL_INFO
#define PSG_LOG_INFO_MSG(message) psg_log_write(PSG_LOG_INFO, (message))
#else
#define PSG_LOG_INFO_MSG(message) ((void)0)
#endif

#if PSG_LOG_LEVEL >= PSG_LOG_LEVEL_DEBUG
#define PSG_LOG_DEBUG_MSG(message) psg_log_write(PSG_LOG_DEBUG, (message))
#else
#define PSG_LOG_DEBUG_MSG(message) ((void)0)
#endif

#define PSG_LOG_WARN_MSG(message) psg_log_write(PSG_LOG_WARN, (message))
#define PSG_LOG_ERROR_MSG(message) psg_log_write(PSG_LOG_ERROR, (message))

#endif /* PSG_UTILS_PSG_LOG_H */
