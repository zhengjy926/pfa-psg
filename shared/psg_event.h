/*
 * Board-to-task event contract (architecture section 1, psg_event.h).
 *
 * Every event carries the hardware facts needed to attribute it to a run,
 * group and burst, plus the monotonic receive/sample timestamp.  Consumers
 * never infer ownership from their own cached run state.
 */
#ifndef PSG_SHARED_PSG_EVENT_H
#define PSG_SHARED_PSG_EVENT_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    PSG_EVENT_SEQUENCE_END = 0, /* TIMA repeat: end of one pulse burst */
    PSG_EVENT_DMA_READY = 1,    /* ADC half/full transfer ready hint */
    PSG_EVENT_OVERCURRENT = 2,  /* comparator trip or latch assertion */
    PSG_EVENT_PORT_EDGE = 3,    /* PB4 start/stop transition captured by CPT1 */
    PSG_EVENT_TYPE_COUNT = 4
} psg_event_type_t;

typedef struct {
    psg_event_type_t type;
    uint32_t run_sequence;
    uint8_t group_index;
    uint16_t burst_index;
    uint64_t timestamp_us;
    uint32_t sample_boundary;
    uint8_t channel_mask;
    uint8_t source;
} psg_event_t;

/* Queue overflow and merge counters exported for 0x31 / RTT error logs (N-005). */
typedef struct {
    uint32_t sequence_end_overflow;
    uint32_t dma_ready_merged;
    uint32_t protection_overflow;
    uint32_t uart_rx_overflow;
} psg_event_counters_t;

#endif /* PSG_SHARED_PSG_EVENT_H */
