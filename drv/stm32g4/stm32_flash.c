/*
 * STM32G4 bank2 record flash.  Reads are memory-mapped; erase/program land
 * with the identity/record ticket (ADR 0002), so this build reports failure
 * rather than pretending a write happened (E-007/E-009).
 */
#include "drv/stm32g4/stm32_flash.h"

#include "bsp/psg_v3/psg_board_config.h"

static flash_status_t flash_read_impl(void *context, uint32_t offset, uint8_t *out, size_t length)
{
    const uint8_t *source;
    size_t index;
    (void)context;

    if ((out == NULL) || ((offset + length) > PSG_FLASH_RECORD_BYTES)) {
        return FLASH_ERROR;
    }
    source = (const uint8_t *)(uintptr_t)(PSG_FLASH_RECORD_BASE + offset);
    for (index = 0u; index < length; index++) {
        out[index] = source[index];
    }
    return FLASH_OK;
}

static bool flash_start_erase_impl(void *context, uint32_t offset)
{
    (void)context;
    (void)offset;
    return false;
}

static bool flash_start_program_impl(void *context, uint32_t offset, const uint8_t *data, size_t length)
{
    (void)context;
    (void)offset;
    (void)data;
    (void)length;
    return false;
}

static bool flash_busy_impl(const void *context)
{
    (void)context;
    return false;
}

static flash_status_t flash_result_impl(const void *context)
{
    (void)context;
    return FLASH_ERROR;
}

static const flash_ops_t g_flash_ops = {
    .read = flash_read_impl,
    .start_erase = flash_start_erase_impl,
    .start_program = flash_start_program_impl,
    .busy = flash_busy_impl,
    .result = flash_result_impl,
};

void stm32_flash_init(stm32_flash_t *impl)
{
    impl->base.ops = &g_flash_ops;
    impl->base.context = impl;
}
