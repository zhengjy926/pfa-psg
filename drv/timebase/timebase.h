/*
 * Monotonic time base device interface (PSG v3).
 *
 * Business code takes every deadline and timestamp from this device.  The
 * FreeRTOS tick is used only for task delays (PRM-728); the two time bases are
 * never mixed.  The device reports 64-bit microseconds; byte receive stamps
 * use the low 32 bits because every deadline derived from them is 50 ms
 * (PRM-115) or 1 s and the 32-bit helpers are wrap-safe.
 */
#ifndef PSG_DRV_TIMEBASE_H
#define PSG_DRV_TIMEBASE_H

#include <stdbool.h>
#include <stdint.h>

typedef struct timebase timebase_t;

typedef struct {
    uint64_t (*now_us)(const timebase_t *timebase);
} timebase_ops_t;

struct timebase {
    const timebase_ops_t *ops;
};

static inline uint64_t timebase_now_us(const timebase_t *timebase)
{
    return timebase->ops->now_us(timebase);
}

/* Wrap-safe 64-bit comparison (PRM-507). */
static inline bool psg_time_reached(uint64_t now_us, uint64_t deadline_us)
{
    return (int64_t)(now_us - deadline_us) >= 0;
}

static inline uint64_t psg_time_add_us(uint64_t time_us, uint64_t delta_us)
{
    return time_us + delta_us;
}

/* Wrap-safe 32-bit comparison for byte receive stamps. */
static inline bool psg_time32_reached(uint32_t now_us, uint32_t deadline_us)
{
    return (int32_t)(now_us - deadline_us) >= 0;
}

static inline bool psg_time32_after(uint32_t later_us, uint32_t earlier_us)
{
    return (int32_t)(later_us - earlier_us) > 0;
}

/* Transmission time of one frame on the configured UART (PRM-002, 10 bits per
 * byte: 1 start + 8 data + 1 stop, no parity). */
#define PSG_UART_BITS_PER_BYTE 10u
#define PSG_UART_BAUD 115200u

static inline uint64_t psg_uart_frame_duration_us(uint32_t frame_bytes)
{
    return ((uint64_t)frame_bytes * (uint64_t)PSG_UART_BITS_PER_BYTE * 1000000ull) /
           (uint64_t)PSG_UART_BAUD;
}

#endif /* PSG_DRV_TIMEBASE_H */
