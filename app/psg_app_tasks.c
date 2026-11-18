#include "app/psg_app_tasks.h"

#include "app/app_config.h"

/* Highest priority first; the monitor must remain schedulable above every
 * supervised task (E-008).  Protocol and UART share the medium band; identity
 * is lowest because Flash steps are long but preemptible. */
const psg_task_def_t psg_task_table[PSG_TASK_COUNT] = {
    {"monitor", 5u, PSG_STACK_MONITOR_BYTES},
    {"pulse", 4u, PSG_STACK_PULSE_BYTES},
    {"settlement", 3u, PSG_STACK_SETTLEMENT_BYTES},
    {"protocol", 2u, PSG_STACK_PROTOCOL_BYTES},
    {"uart", 2u, PSG_STACK_UART_BYTES},
    {"identity", 1u, PSG_STACK_IDENTITY_BYTES},
};
