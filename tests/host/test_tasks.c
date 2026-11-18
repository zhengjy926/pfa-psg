#include "test_support.h"

#include <string.h>

#include "app/psg_app_tasks.h"

TEST(tasks, registers_the_six_architecture_tasks)
{
    CHECK_EQ(PSG_TASK_COUNT, 6u);
    CHECK(strcmp(psg_task_table[PSG_TASK_MONITOR].name, "monitor") == 0);
    CHECK(strcmp(psg_task_table[PSG_TASK_PULSE].name, "pulse") == 0);
    CHECK(strcmp(psg_task_table[PSG_TASK_SETTLEMENT].name, "settlement") == 0);
    CHECK(strcmp(psg_task_table[PSG_TASK_PROTOCOL].name, "protocol") == 0);
    CHECK(strcmp(psg_task_table[PSG_TASK_UART].name, "uart") == 0);
    CHECK(strcmp(psg_task_table[PSG_TASK_IDENTITY].name, "identity") == 0);
}

TEST(tasks, monitor_is_highest_and_identity_lowest)
{
    uint8_t index;

    for (index = 1u; index < (uint8_t)PSG_TASK_COUNT; index++) {
        CHECK(psg_task_table[PSG_TASK_MONITOR].priority > psg_task_table[index].priority);
    }
    for (index = 0u; index < (uint8_t)PSG_TASK_IDENTITY; index++) {
        CHECK(psg_task_table[PSG_TASK_IDENTITY].priority < psg_task_table[index].priority);
    }
}

TEST(tasks, every_task_has_a_stack_large_enough_for_the_canary)
{
    uint8_t index;

    for (index = 0u; index < (uint8_t)PSG_TASK_COUNT; index++) {
        CHECK(psg_task_table[index].stack_bytes >= 512u);
        CHECK(psg_task_table[index].stack_bytes <= 1024u);
    }
}
