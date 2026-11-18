/*
 * Shared compile-time values (addresses, frame limits, timing, buffer sizes).
 *
 * This header is the neutral half of app/app_config.h: run, svc, drv and bsp
 * include it directly so no lower layer depends on app/.  Application-only
 * knobs (log level, task stack sizes, queue depths) stay in app/app_config.h.
 */
#ifndef PSG_SHARED_PSG_CONFIG_H
#define PSG_SHARED_PSG_CONFIG_H

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
#define PSG_HANDSHAKE_REPORT_INTERVAL_US 1000000ull

/* Dedup (PRM-027..PRM-029). */
#define PSG_DEDUP_DEPTH 8u
#define PSG_DEDUP_ENTRY_BYTES 16u
#define PSG_DEDUP_PAYLOAD_BYTES 11u

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

/* Log level (N-005): 0 error, 1 warn, 2 info, 3 debug.  Release keeps ERROR
 * and WARN; debug builds raise this to 3. */
#ifndef PSG_LOG_LEVEL
#define PSG_LOG_LEVEL 1
#endif

#endif /* PSG_SHARED_PSG_CONFIG_H */
