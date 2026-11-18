#include "fakes/fake_flash.h"

#include <string.h>

static flash_status_t fake_flash_read(void *context, uint32_t offset, uint8_t *out, size_t length)
{
    fake_flash_t *fake = (fake_flash_t *)context;

    if ((offset + length) > FAKE_FLASH_BYTES) {
        return FLASH_ERROR;
    }
    (void)memcpy(out, &fake->memory[offset], length);
    return FLASH_OK;
}

static bool fake_flash_start_erase(void *context, uint32_t offset)
{
    fake_flash_t *fake = (fake_flash_t *)context;

    if (offset >= FAKE_FLASH_BYTES) {
        return false;
    }
    (void)memset(&fake->memory[offset], 0xFF, 2048u);
    fake->result = FLASH_OK;
    return true;
}

static bool fake_flash_start_program(void *context, uint32_t offset, const uint8_t *data, size_t length)
{
    fake_flash_t *fake = (fake_flash_t *)context;

    if ((offset + length) > FAKE_FLASH_BYTES) {
        return false;
    }
    (void)memcpy(&fake->memory[offset], data, length);
    fake->result = FLASH_OK;
    return true;
}

static bool fake_flash_busy(const void *context)
{
    const fake_flash_t *fake = (const fake_flash_t *)context;

    return fake->busy;
}

static flash_status_t fake_flash_result(const void *context)
{
    const fake_flash_t *fake = (const fake_flash_t *)context;

    return fake->result;
}

static const flash_ops_t g_flash_ops = {
    .read = fake_flash_read,
    .start_erase = fake_flash_start_erase,
    .start_program = fake_flash_start_program,
    .busy = fake_flash_busy,
    .result = fake_flash_result,
};

void fake_flash_init(fake_flash_t *fake)
{
    (void)memset(fake, 0, sizeof(*fake));
    (void)memset(fake->memory, 0xFF, sizeof(fake->memory));
    fake->base.ops = &g_flash_ops;
    fake->base.context = fake;
    fake->busy = false;
    fake->result = FLASH_OK;
}
