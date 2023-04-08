// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2005-20111 Michael Noland (joat)
// Copyright (C) 2005-20111 Jason Rogers (Dovoto)
// Copyright (C) 2005-20111 Dave Murphy (WinterMute)

#include <nds/ndstypes.h>
#include <nds/system.h>
#include <nds/arm7/i2c.h>

void systemShutDown()
{
	if (!isDSiMode()) {
		writePowerManagement(PM_CONTROL_REG,PM_SYSTEM_PWR);
	} else {
		i2cWriteRegister(I2C_PM, I2CREGPM_RESETFLAG, 1);
		i2cWriteRegister(I2C_PM, I2CREGPM_PWRCNT, 1);
	}
}
