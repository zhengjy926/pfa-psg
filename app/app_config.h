/*
 * Compile-time configuration for PSG v3 (SRS N-001, A-02 in architecture).
 *
 * Every macro that carries a PRM value names it.  Changing a value is a
 * regression trigger (architecture section 2).
 */
#ifndef PSG_APP_CONFIG_H
#define PSG_APP_CONFIG_H

#include <stdint.h>

/* Identity and addressing (PRM-201..PRM-206). */
#define PSG_PRODUCT_ADDRESS 0x03u
#define PSG_BOARD_ADDRESS 0x02u
#define PSG_VERSION_BYTES 7u
#define PSG_SOFTWARE_VERSION "0.0.0.0"
#define PSG_HARDWARE_VERSION_DEFAULT "0.0.0.0"
#define PSG_SERIAL_NUMBER_BYTES 12u
#define PSG_SERIAL_NUMBER_DEFAULT "SN0000000000"

/* ECSP framing (PRM-007..PRM-023). */
#define PSG_ECSP_HEAD 0xFAu
#define PSG_ECSP_TAIL 0x0Du
#define PSG_ECSP_MIN_FRAME_BYTES 9u
#define PSG_ECSP_MAX_FRAME_BYTES 450u
#define PSG_ECSP_MAX_PAYLOAD_BYTES 441u
#define PSG_ECSP_HEADER_BYTES 6u
#define PSG_ECSP_CRC_BYTES 2u

/* Timing (PRM-101..PRM-120). */
#define PSG_HALF_PACKET_TIMEOUT_US 50000ull
#define PSG_HEARTBEAT_INTERVAL_US 1000000ull
#define PSG_FIRST_HANDSHAKE_DEADLINE_US 500000ull
#define PSG_HANDSHAKE_REPORT_INTERVAL_US 1000000ull

/* Dedup (PRM-027..PRM-029). */
#define PSG_DEDUP_DEPTH 8u
#define PSG_DEDUP_ENTRY_BYTES 16u
#define PSG_DEDUP_PAYLOAD_BYTES 12u

/* Port selection and buffers (PRM-011, PRM-736). */
#define PSG_PROTO_UART_INSTANCE 2u
#define PSG_UART_RX_TASK_BATCH_BYTES 64u
#define PSG_ECSP_RX_WINDOW_BYTES 512u
#define PSG_ECSP_TX_FRAME_BYTES PSG_ECSP_MAX_FRAME_BYTES
#define PSG_ECSP_REPLY_QUEUE_DEPTH 4u
#define PSG_ECSP_EVENT_QUEUE_DEPTH 4u

/* Settlement wake period (PRM-117) and watchdog (PRM-607/PRM-608). */
#define PSG_SETTLEMENT_WAKE_PERIOD_MS 10u
#define PSG_TASK_MONITOR_PERIOD_MS 100u
#define PSG_WATCHDOG_MISS_LIMIT 2u
#define PSG_LED_TOGGLE_PERIOD_MS 500u

/* Task stack sizes in bytes; configCHECK_FOR_STACK_OVERFLOW=2 guards them. */
#define PSG_STACK_MONITOR_BYTES 512u
#define PSG_STACK_PULSE_BYTES 768u
#define PSG_STACK_SETTLEMENT_BYTES 512u
#define PSG_STACK_PROTOCOL_BYTES 1024u
#define PSG_STACK_UART_BYTES 512u
#define PSG_STACK_IDENTITY_BYTES 512u

/* Log level (N-005): 0 error, 1 warn, 2 info, 3 debug. */
#ifndef PSG_LOG_LEVEL
#define PSG_LOG_LEVEL 0
#endif

#endif /* PSG_APP_CONFIG_H */
