/*
 * Non-volatile flash device interface (ADR 0002, SRS E-009).
 *
 * Read is synchronous.  Erase and program are started and then polled from a
 * service task; no caller spins on the controller.
 */
#ifndef PSG_DRV_FLASH_H
#define PSG_DRV_FLASH_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum {
    FLASH_OK = 0,
    FLASH_BUSY = 1,
    FLASH_ERROR = 2
} flash_status_t;

typedef struct flash flash_t;

typedef struct {
    flash_status_t (*read)(void *context, uint32_t offset, uint8_t *out, size_t length);
    bool (*start_erase)(void *context, uint32_t offset);
    bool (*start_program)(void *context, uint32_t offset, const uint8_t *data, size_t length);
    bool (*busy)(const void *context);
    flash_status_t (*result)(const void *context);
} flash_ops_t;

struct flash {
    const flash_ops_t *ops;
    void *context;
};

static inline flash_status_t flash_read(const flash_t *flash, uint32_t offset, uint8_t *out, size_t length)
{
    return flash->ops->read(flash->context, offset, out, length);
}

static inline bool flash_start_erase(const flash_t *flash, uint32_t offset)
{
    return flash->ops->start_erase(flash->context, offset);
}

static inline bool flash_start_program(const flash_t *flash, uint32_t offset, const uint8_t *data, size_t length)
{
    return flash->ops->start_program(flash->context, offset, data, length);
}

static inline bool flash_busy(const flash_t *flash)
{
    return flash->ops->busy(flash->context);
}

static inline flash_status_t flash_result(const flash_t *flash)
{
    return flash->ops->result(flash->context);
}

#endif /* PSG_DRV_FLASH_H */
