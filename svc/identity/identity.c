#include "svc/identity/identity.h"

#include <string.h>

void identity_init(identity_t *identity, const flash_t *flash)
{
    (void)flash;
    (void)memcpy(identity->hardware_version, PSG_HARDWARE_VERSION_DEFAULT, PSG_VERSION_BYTES);
    (void)memcpy(identity->serial, PSG_SERIAL_NUMBER_DEFAULT, PSG_SERIAL_NUMBER_BYTES);
    identity->hardware_version_persisted = false;
    identity->serial_persisted = false;
}

const uint8_t *identity_software_version(void)
{
    return (const uint8_t *)PSG_SOFTWARE_VERSION;
}

const uint8_t *identity_hardware_version(const identity_t *identity)
{
    return identity->hardware_version;
}

const uint8_t *identity_serial_number(const identity_t *identity)
{
    return identity->serial;
}
