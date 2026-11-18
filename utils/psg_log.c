#include "utils/psg_log.h"

#include <stddef.h>

/* Backend hook: weak so the target image links before RTT is initialised.
 * The board may override it by defining psg_log_backend_write(). */
__attribute__((weak)) void psg_log_backend_write(psg_log_level_t level, const char *message)
{
    (void)level;
    (void)message;
}

void psg_log_write(psg_log_level_t level, const char *message)
{
    if (message == NULL) {
        return;
    }
    if ((int)level > (int)PSG_LOG_LEVEL) {
        return;
    }
    psg_log_backend_write(level, message);
}
