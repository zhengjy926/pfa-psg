/*
 * Identity records: software version (compile-time), hardware version and
 * serial number (PRM-201..PRM-207, SRS E-009).
 *
 * This increment exposes the read path with the documented power-up defaults;
 * the A/B Flash record service lands with the identity ticket and replaces
 * identity_init()'s body, not its interface.
 */
#ifndef PSG_SVC_IDENTITY_IDENTITY_H
#define PSG_SVC_IDENTITY_IDENTITY_H

#include <stdbool.h>
#include <stdint.h>

#include "app/app_config.h"
#include "drv/flash/flash.h"

typedef struct {
    uint8_t hardware_version[PSG_VERSION_BYTES];
    uint8_t serial[PSG_SERIAL_NUMBER_BYTES];
    bool hardware_version_persisted;
    bool serial_persisted;
} identity_t;

void identity_init(identity_t *identity, const flash_t *flash);

const uint8_t *identity_software_version(void);
const uint8_t *identity_hardware_version(const identity_t *identity);
const uint8_t *identity_serial_number(const identity_t *identity);

#endif /* PSG_SVC_IDENTITY_IDENTITY_H */
