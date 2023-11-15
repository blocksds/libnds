// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2011 Dave Murphy (WinterMute)

// I2C control for the ARM7

#include <nds/arm7/i2c.h>
#include <nds/bios.h>

static u32 i2cCurrentDelay = 0;

void i2cDelay(void)
{
    i2cWaitBusy();
    swiDelay(i2cCurrentDelay);
}

void i2cStop(u8 arg0)
{
    if (i2cCurrentDelay)
    {
        REG_I2CCNT = arg0 | I2CCNT_ENABLE | I2CCNT_ENABLE_IRQ;
        i2cDelay();
        REG_I2CCNT = I2CCNT_ENABLE | I2CCNT_ENABLE_IRQ | I2CCNT_ERROR | I2CCNT_STOP;
    }
    else
    {
        REG_I2CCNT = arg0 | I2CCNT_ENABLE | I2CCNT_ENABLE_IRQ | I2CCNT_STOP;
    }
}

u8 i2cGetResult(void)
{
    i2cWaitBusy();
    return (REG_I2CCNT >> 4) & 0x01;
}

u8 i2cGetData(void)
{
    i2cWaitBusy();
    return REG_I2CDATA;
}

void i2cSetDelay(u8 device)
{
    if (device == I2C_PM)
        i2cCurrentDelay = 0x180;
    else
        i2cCurrentDelay = 0;
}

u8 i2cSelectDevice(u8 device)
{
    i2cWaitBusy();
    REG_I2CDATA = device;
    REG_I2CCNT = I2CCNT_ENABLE | I2CCNT_ENABLE_IRQ | I2CCNT_START;
    return i2cGetResult();
}

u8 i2cSelectRegister(u8 reg)
{
    i2cDelay();
    REG_I2CDATA = reg;
    REG_I2CCNT = I2CCNT_ENABLE | I2CCNT_ENABLE_IRQ;
    return i2cGetResult();
}

u8 i2cWriteRegister(u8 device, u8 reg, u8 data)
{
    i2cSetDelay(device);

    for (int i = 0; i < 8; i++)
    {
        if ((i2cSelectDevice(device) != 0) && (i2cSelectRegister(reg) != 0))
        {
            i2cDelay();
            REG_I2CDATA = data;
            i2cStop(I2CCNT_WRITE);
            if (i2cGetResult() != 0)
                return 1;
        }
        REG_I2CCNT = I2CCNT_ENABLE | I2CCNT_ENABLE_IRQ | I2CCNT_STOP | I2CCNT_ERROR;
    }

    return 0;
}

u8 i2cReadRegister(u8 device, u8 reg)
{
    i2cSetDelay(device);

    for (int i = 0; i < 8; i++)
    {
        if ((i2cSelectDevice(device) != 0) && (i2cSelectRegister(reg) != 0))
        {
            i2cDelay();
            if (i2cSelectDevice(device | 1))
            {
                i2cDelay();
                i2cStop(I2CCNT_READ);
                return i2cGetData();
            }
        }

        REG_I2CCNT = I2CCNT_ENABLE | I2CCNT_ENABLE_IRQ | I2CCNT_STOP | I2CCNT_ERROR;
    }

    return 0xff;
}
