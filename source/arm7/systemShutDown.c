// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2005-2011 Michael Noland (joat)
// Copyright (C) 2005-2011 Jason Rogers (dovoto)
// Copyright (C) 2005-2011 Dave Murphy (WinterMute)
// Copyright (C) 2025 Antonio Niño Díaz

#include <nds/arm7/i2c.h>
#include <nds/bios.h>
#include <nds/ndstypes.h>
#include <nds/system.h>

void systemShutDown(void)
{
    writePowerManagement(PM_CONTROL_REG, PM_SYSTEM_PWR);
}

void systemReboot(void)
{
    if (isDSiMode())
    {
        i2cWriteRegister(I2C_PM, I2CREGPM_RESETFLAG, 1);
        i2cWriteRegister(I2C_PM, I2CREGPM_PWRCNT, 1);
        // Wait for 20 ms to give some time for the DS to reboot. This value has
        // been obtained by trial and error. 1 ms doesn't seem to be enough.
        swiDelay(20 * 0x20BA);
    }
}
