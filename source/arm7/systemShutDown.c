// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2005-20111 Michael Noland (joat)
// Copyright (C) 2005-20111 Jason Rogers (Dovoto)
// Copyright (C) 2005-20111 Dave Murphy (WinterMute)

#include <nds/arm7/i2c.h>
#include <nds/ndstypes.h>
#include <nds/system.h>

void systemShutDown(void)
{
    if (!isDSiMode())
    {
        writePowerManagement(PM_CONTROL_REG,PM_SYSTEM_PWR);
    }
    else
    {
        i2cWriteRegister(I2C_PM, I2CREGPM_RESETFLAG, 1);
        i2cWriteRegister(I2C_PM, I2CREGPM_PWRCNT, 1);
    }
}
