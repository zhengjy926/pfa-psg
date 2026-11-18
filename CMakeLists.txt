cmake_minimum_required(VERSION 3.20)

project(psg_v3 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

option(PSG_BUILD_FIRMWARE "Build the STM32G474 target image" OFF)
option(PSG_BUILD_TESTS "Build the host test suite" ON)

if(PSG_BUILD_FIRMWARE)
    add_subdirectory(firmware)
    return()
endif()

set(PSG_WARNINGS
    -Wall
    -Wextra
    -Werror
    -Wshadow
    -Wundef
    -Wstrict-prototypes
    -Wmissing-prototypes
    -Wpointer-arith
    -Wcast-align
    -Wwrite-strings
)

set(PSG_CORE_SOURCES
    app/psg_app_tasks.c
    app/psg_ecsp.c
    app/psg_reports.c
    app/ecsp/ecsp_frame.c
    app/ecsp/ecsp_rx.c
    app/ecsp/ecsp_session.c
    app/ecsp/ecsp_tx.c
    drv/uart/uart_ring.c
    run/pulse_engine.c
    run/run_config.c
    shared/psg_bytes.c
    svc/dedup/dedup.c
    svc/fault/fault.c
    svc/identity/identity.c
    utils/psg_log.c
)

add_library(psg_core STATIC ${PSG_CORE_SOURCES})
target_include_directories(psg_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_options(psg_core PRIVATE ${PSG_WARNINGS})

if(PSG_BUILD_TESTS)
    enable_testing()

    add_executable(psg_host_tests
        tests/host/test_main.c
        tests/host/psg_fixture.c
        tests/host/fakes/fake_timebase.c
        tests/host/fakes/fake_uart.c
        tests/host/fakes/fake_board.c
        tests/host/fakes/fake_led.c
        tests/host/fakes/fake_flash.c
        tests/host/test_frame.c
        tests/host/test_uart_ring.c
        tests/host/test_ecsp_rx.c
        tests/host/test_session.c
        tests/host/test_dispatch.c
        tests/host/test_dedup.c
        tests/host/test_run.c
        tests/host/test_upload.c
        tests/host/test_tasks.c
    )
    target_include_directories(psg_host_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/tests/host)
    target_link_libraries(psg_host_tests PRIVATE psg_core)
    target_compile_options(psg_host_tests PRIVATE ${PSG_WARNINGS})
    target_compile_definitions(psg_host_tests PRIVATE __USE_MINGW_ANSI_STDIO=1)

    add_test(NAME psg_host_tests COMMAND psg_host_tests)
endif()
