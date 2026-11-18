# PSG v3 target image (STM32G474QET6).
#
# Two toolchains consume these sources: the Keil MDK-ARM/armclang project in
# keil/ and this CMake build with arm-none-eabi-gcc.  The CMake build is the
# CI-verifiable one; the Keil project is the released IDE build.

set(PSG_FREERTOS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../third_party/freertos")
set(PSG_CMSIS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../third_party/cmsis")

enable_language(ASM)

add_library(psg_third_party STATIC
    ${PSG_FREERTOS_DIR}/Source/tasks.c
    ${PSG_FREERTOS_DIR}/Source/queue.c
    ${PSG_FREERTOS_DIR}/Source/list.c
    ${PSG_FREERTOS_DIR}/Source/portable/GCC/ARM_CM4F/port.c
    ${PSG_CMSIS_DIR}/device/src/system_stm32g4xx.c
)
target_include_directories(psg_third_party PUBLIC
    ${PSG_FREERTOS_DIR}/Source/include
    ${PSG_FREERTOS_DIR}/Source/portable/GCC/ARM_CM4F
    ${PSG_CMSIS_DIR}/include
    ${PSG_CMSIS_DIR}/device/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../bsp/psg_v3
)
target_compile_options(psg_third_party PRIVATE ${PSG_ARM_CPU_FLAGS} -O2 -g3 -ffunction-sections -fdata-sections)
target_compile_definitions(psg_third_party PUBLIC STM32G474xx HSE_VALUE=8000000u)

add_executable(psg_v3.elf
    ${CMAKE_CURRENT_SOURCE_DIR}/../app/main.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../app/psg_app.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../app/psg_app_tasks.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../app/psg_ecsp.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../app/psg_reports.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../app/ecsp/ecsp_frame.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../app/ecsp/ecsp_rx.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../app/ecsp/ecsp_session.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../app/ecsp/ecsp_tx.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../run/pulse_engine.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../run/run_config.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../svc/dedup/dedup.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../svc/fault/fault.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../svc/identity/identity.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../drv/uart/uart_ring.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../drv/stm32g4/stm32_timebase.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../drv/stm32g4/stm32_led.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../drv/stm32g4/stm32_uart.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../drv/stm32g4/stm32_comp.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../drv/stm32g4/stm32_iwdg.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../drv/stm32g4/stm32_watchdog.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../drv/stm32g4/stm32_flash.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../bsp/psg_v3/psg_board.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../utils/psg_log.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../utils/syscalls.c
    ${PSG_CMSIS_DIR}/startup/gcc/startup_stm32g474xx.s
)

target_include_directories(psg_v3.elf PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_compile_options(psg_v3.elf PRIVATE
    ${PSG_ARM_CPU_FLAGS}
    -std=c11
    -O2
    -g3
    -ffunction-sections
    -fdata-sections
    -fno-common
    -Wall
    -Wextra
    -Werror
    -Wshadow
    -Wundef
    -Wstrict-prototypes
    -Wmissing-prototypes
    -Wpointer-arith
    -Wcast-align
)
target_link_options(psg_v3.elf PRIVATE
    ${PSG_ARM_CPU_FLAGS}
    -T${CMAKE_CURRENT_SOURCE_DIR}/../bsp/psg_v3/psg_v3.ld
    -Wl,--gc-sections
    -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/psg_v3.map
    -Wl,--print-memory-usage
    -Wl,--no-warn-rwx-segments
    -specs=nosys.specs
    -specs=nano.specs
)
target_link_libraries(psg_v3.elf PRIVATE psg_third_party)

add_custom_command(TARGET psg_v3.elf POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:psg_v3.elf> ${CMAKE_CURRENT_BINARY_DIR}/psg_v3.hex
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:psg_v3.elf> ${CMAKE_CURRENT_BINARY_DIR}/psg_v3.bin
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:psg_v3.elf>
)
