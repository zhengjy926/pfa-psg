#include "app/psg_app.h"

#include "FreeRTOS.h"
#include "app/app_config.h"
#include "app/psg_app_tasks.h"
#include "app/psg_ecsp.h"
#include "bsp/psg_v3/psg_board.h"
#include "queue.h"
#include "run/pulse_engine.h"
#include "svc/fault/fault.h"
#include "svc/identity/identity.h"
#include "task.h"

#define UART_BATCH_BYTES 64u
#define UART_BATCH_DEPTH 8u

typedef struct {
    uint8_t count;
    uint8_t data[UART_BATCH_BYTES];
    uint32_t stamps[UART_BATCH_BYTES];
} psg_uart_batch_t;

static psg_board_devices_t *g_devices;
static psg_fault_t g_fault;
static identity_t g_identity;
static psg_run_t g_run;
static psg_ecsp_t g_ecsp;

static StaticQueue_t g_uart_queue_control;
static uint8_t g_uart_queue_storage[UART_BATCH_DEPTH * sizeof(psg_uart_batch_t)];
static QueueHandle_t g_uart_queue;

static StaticQueue_t g_settlement_queue_control;
static uint8_t g_settlement_queue_storage[2u * sizeof(uint32_t)];
static QueueHandle_t g_settlement_queue;

static volatile uint32_t g_progress[PSG_TASK_COUNT];

static StaticTask_t g_task_control[PSG_TASK_COUNT];
static StackType_t g_monitor_stack[PSG_STACK_MONITOR_BYTES / sizeof(StackType_t)];
static StackType_t g_pulse_stack[PSG_STACK_PULSE_BYTES / sizeof(StackType_t)];
static StackType_t g_settlement_stack[PSG_STACK_SETTLEMENT_BYTES / sizeof(StackType_t)];
static StackType_t g_protocol_stack[PSG_STACK_PROTOCOL_BYTES / sizeof(StackType_t)];
static StackType_t g_uart_stack[PSG_STACK_UART_BYTES / sizeof(StackType_t)];
static StackType_t g_identity_stack[PSG_STACK_IDENTITY_BYTES / sizeof(StackType_t)];

static StaticTask_t g_idle_control;
static StackType_t g_idle_stack[configMINIMAL_STACK_SIZE];

static void task_monitor(void *argument);
static void task_pulse(void *argument);
static void task_settlement(void *argument);
static void task_protocol(void *argument);
static void task_uart(void *argument);
static void task_identity(void *argument);

static const TaskFunction_t g_task_entries[PSG_TASK_COUNT] = {
    task_monitor, task_pulse, task_settlement, task_protocol, task_uart, task_identity,
};

static StackType_t *const g_task_stacks[PSG_TASK_COUNT] = {
    g_monitor_stack, g_pulse_stack, g_settlement_stack, g_protocol_stack, g_uart_stack, g_identity_stack,
};

void vApplicationGetIdleTaskMemory(StaticTask_t **tcb, StackType_t **stack, uint32_t *size)
{
    *tcb = &g_idle_control;
    *stack = g_idle_stack;
    *size = (uint32_t)configMINIMAL_STACK_SIZE;
}

void vApplicationStackOverflowHook(TaskHandle_t task, char *name)
{
    (void)task;
    (void)name;
    taskDISABLE_INTERRUPTS();
    for (;;) {
    }
}

void psg_rtos_assert_failed(void)
{
    taskDISABLE_INTERRUPTS();
    for (;;) {
    }
}

static void uart_to_protocol_queue(void)
{
    uart_rx_byte_t bytes[UART_BATCH_BYTES];
    uint32_t gaps;
    size_t count;

    count = uart_read(&g_devices->uart.base, bytes, UART_BATCH_BYTES);
    gaps = uart_take_gaps(&g_devices->uart.base);
    if ((count != 0u) || (gaps != 0u)) {
        psg_uart_batch_t batch;
        size_t index;

        if (count > UART_BATCH_BYTES) {
            count = UART_BATCH_BYTES;
        }
        batch.count = (uint8_t)count;
        for (index = 0u; index < count; index++) {
            batch.data[index] = bytes[index].value;
            batch.stamps[index] = bytes[index].timestamp_us;
        }
        if (xQueueSend(g_uart_queue, &batch, 0u) != pdPASS) {
            /* The driver ring keeps producing; losing a batch forces the
             * parser to resynchronise rather than silently dropping bytes. */
            ecsp_rx_note_gap(&g_ecsp.rx);
        }
    }
}

static void task_uart(void *argument)
{
    (void)argument;

    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(5));
        uart_to_protocol_queue();
        g_progress[PSG_TASK_UART]++;
    }
}

static void task_protocol(void *argument)
{
    (void)argument;

    for (;;) {
        psg_uart_batch_t batch;
        BaseType_t received = xQueueReceive(g_uart_queue, &batch, pdMS_TO_TICKS(5));

        while (received == pdPASS) {
            uart_rx_byte_t bytes[UART_BATCH_BYTES];
            size_t index;

            for (index = 0u; index < batch.count; index++) {
                bytes[index].value = batch.data[index];
                bytes[index].timestamp_us = batch.stamps[index];
            }
            psg_ecsp_feed(&g_ecsp, bytes, batch.count, 0u);
            received = xQueueReceive(g_uart_queue, &batch, 0u);
        }
        psg_ecsp_step(&g_ecsp, timebase_now_us(&g_devices->timebase.base));
        g_progress[PSG_TASK_PROTOCOL]++;
    }
}

static void task_pulse(void *argument)
{
    (void)argument;

    for (;;) {
        /* Waveform execution and board events land with the pulse-engine
         * ticket; the task slot and its cadence are reserved here. */
        vTaskDelay(pdMS_TO_TICKS(1));
        g_progress[PSG_TASK_PULSE]++;
    }
}

static void task_settlement(void *argument)
{
    uint32_t event;
    (void)argument;

    for (;;) {
        (void)xQueueReceive(g_settlement_queue, &event, pdMS_TO_TICKS(PSG_SETTLEMENT_WAKE_PERIOD_MS));
        g_progress[PSG_TASK_SETTLEMENT]++;
    }
}

static void task_identity(void *argument)
{
    (void)argument;

    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(100));
        g_progress[PSG_TASK_IDENTITY]++;
    }
}

static void task_monitor(void *argument)
{
    uint32_t led_divider = 0u;
    bool led_on = false;
    (void)argument;

    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(PSG_TASK_MONITOR_PERIOD_MS));
        led_divider++;
        if (led_divider >= (PSG_LED_TOGGLE_PERIOD_MS / PSG_TASK_MONITOR_PERIOD_MS)) {
            led_divider = 0u;
            led_on = !led_on;
            led_set(&g_devices->led.base, led_on);
        }
        g_progress[PSG_TASK_MONITOR]++;
    }
}

void psg_app_init(void)
{
    psg_ecsp_config_t config;

    g_devices = psg_board_devices();
    psg_fault_init(&g_fault);
    identity_init(&g_identity, &g_devices->flash.base);
    psg_run_init(&g_run, g_devices->board, &g_fault);

    g_uart_queue = xQueueCreateStatic(UART_BATCH_DEPTH, sizeof(psg_uart_batch_t), g_uart_queue_storage,
                                      &g_uart_queue_control);
    g_settlement_queue = xQueueCreateStatic(2u, sizeof(uint32_t), g_settlement_queue_storage,
                                            &g_settlement_queue_control);

    config.uart = &g_devices->uart.base;
    config.board = g_devices->board;
    config.run = &g_run;
    config.identity = &g_identity;
    config.fault = &g_fault;
    config.product_address = PSG_PRODUCT_ADDRESS;
    config.board_address = PSG_BOARD_ADDRESS;
    psg_ecsp_init(&g_ecsp, &config, timebase_now_us(&g_devices->timebase.base));
}

void psg_app_start(void)
{
    uint8_t index;

    for (index = 0u; index < (uint8_t)PSG_TASK_COUNT; index++) {
        TaskHandle_t handle = xTaskCreateStatic(g_task_entries[index], psg_task_table[index].name,
                                                (configSTACK_DEPTH_TYPE)(psg_task_table[index].stack_bytes /
                                                                         sizeof(StackType_t)),
                                                NULL, psg_task_table[index].priority,
                                                g_task_stacks[index], &g_task_control[index]);
        if (handle == NULL) {
            taskDISABLE_INTERRUPTS();
            for (;;) {
            }
        }
    }
    vTaskStartScheduler();
    for (;;) {
    }
}
