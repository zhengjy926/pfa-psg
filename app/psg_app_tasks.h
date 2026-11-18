/*
 * Six-task registry (architecture section 1, ADR 0001).
 */
#ifndef PSG_APP_PSG_APP_TASKS_H
#define PSG_APP_PSG_APP_TASKS_H

#include <stdint.h>

typedef enum {
    PSG_TASK_MONITOR = 0,
    PSG_TASK_PULSE = 1,
    PSG_TASK_SETTLEMENT = 2,
    PSG_TASK_PROTOCOL = 3,
    PSG_TASK_UART = 4,
    PSG_TASK_IDENTITY = 5,
    PSG_TASK_COUNT = 6
} psg_task_id_t;

typedef struct {
    const char *name;
    uint8_t priority;
    uint16_t stack_bytes;
} psg_task_def_t;

extern const psg_task_def_t psg_task_table[PSG_TASK_COUNT];

#endif /* PSG_APP_PSG_APP_TASKS_H */
