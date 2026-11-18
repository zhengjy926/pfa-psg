/*
 * FreeRTOS configuration for PSG v3 (PRM-727/PRM-728, E-008).
 *
 * Static allocation only: N-001 forbids dynamic memory, so every task, queue
 * and TCB lives in a named static object.
 */
#ifndef PSG_FREERTOS_CONFIG_H
#define PSG_FREERTOS_CONFIG_H

#define configUSE_PREEMPTION 1
#define configUSE_TIME_SLICING 1
#define configUSE_IDLE_HOOK 0
#define configUSE_TICK_HOOK 0
#define configUSE_TICKLESS_IDLE 0
#define configCPU_CLOCK_HZ 170000000u
#define configTICK_RATE_HZ 1000u
#define configMAX_PRIORITIES 8
#define configMINIMAL_STACK_SIZE 128u
#define configMAX_TASK_NAME_LEN 16
#define configUSE_16_BIT_TICKS 0
#define configIDLE_SHOULD_YIELD 1
#define configUSE_TASK_NOTIFICATIONS 1
#define configTASK_NOTIFICATION_ARRAY_ENTRIES 1
#define configUSE_MUTEXES 1
#define configUSE_RECURSIVE_MUTEXES 0
#define configUSE_COUNTING_SEMAPHORES 0
#define configQUEUE_REGISTRY_SIZE 0
#define configUSE_QUEUE_SETS 0
#define configUSE_TIMERS 0
#define configSUPPORT_STATIC_ALLOCATION 1
#define configSUPPORT_DYNAMIC_ALLOCATION 0
#define configAPPLICATION_ALLOCATED_HEAP 0
#define configUSE_MALLOC_FAILED_HOOK 0
#define configCHECK_FOR_STACK_OVERFLOW 2
#define configUSE_TRACE_FACILITY 0
#define configGENERATE_RUN_TIME_STATS 0
#define configUSE_NEWLIB_REENTRANT 0
#define configUSE_PORT_OPTIMISED_TASK_SELECTION 1
#define configUSE_C_RUNTIME_TLS_SUPPORT 0
#define configNUMBER_OF_CORES 1

#define INCLUDE_vTaskDelay 1
#define INCLUDE_vTaskDelayUntil 1
#define INCLUDE_xTaskGetCurrentTaskHandle 1
#define INCLUDE_xTaskGetSchedulerState 1

/* PRM-727: priorities 0-3 stay unused; 4 is the highest usable value. */
#define configPRIO_BITS 4
#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY 15
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY 4
#define configKERNEL_INTERRUPT_PRIORITY \
    (configLIBRARY_LOWEST_INTERRUPT_PRIORITY << (8 - configPRIO_BITS))
#define configMAX_SYSCALL_INTERRUPT_PRIORITY \
    (configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << (8 - configPRIO_BITS))

void psg_rtos_assert_failed(void);
#define configASSERT(condition) \
    do {                        \
        if ((condition) == 0) { \
            psg_rtos_assert_failed(); \
        }                       \
    } while (0)

/* Route the Cortex-M4 exception vectors to the FreeRTOS port. */
#define vPortSVCHandler SVC_Handler
#define xPortPendSVHandler PendSV_Handler
#define xPortSysTickHandler SysTick_Handler

#endif /* PSG_FREERTOS_CONFIG_H */
