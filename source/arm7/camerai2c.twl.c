// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2011 Dave Murphy (WinterMute)
// Copyright (C) 2023 Epicpkmn11

// I2C control for the ARM7, modified for Aptina camera interfacing

// Based on libnds's i2c.twl.c, but modifed to work with u16 addresses and data
// for the Aptina cameras.
//
// TODO: Merge back with i2c.twl.c.

#include <nds/arm7/camera.h>
#include <nds/arm7/i2c.h>
#include <nds/bios.h>

void i2cDelay(void);
void i2cStop(u8 arg0);
u8 i2cGetResult(void);
void i2cSetDelay(u8 device);

enum i2cFlags
{
    I2C_NONE = 0x00,
    I2C_STOP = 0x01,
    I2C_START = 0x02,
    I2C_ACK = 0x10,
    I2C_READ = 0x20
};

u8 aptGetData(u8 flags)
{
    REG_I2CCNT = I2CCNT_ENABLE | I2CCNT_ENABLE_IRQ | flags;
    i2cWaitBusy();
    return REG_I2CDATA;
}

u8 aptSetData(u8 data, u8 flags)
{
    REG_I2CDATA = data;
    REG_I2CCNT = I2CCNT_ENABLE | I2CCNT_ENABLE_IRQ | flags;
    return i2cGetResult();
}

u8 aptSelectDevice(u8 device, u8 flags)
{
    i2cWaitBusy();
    REG_I2CDATA = device;
    REG_I2CCNT = I2CCNT_ENABLE | I2CCNT_ENABLE_IRQ | flags;
    return i2cGetResult();
}

u8 aptSelectRegister(u8 reg, u8 flags)
{
    i2cDelay();
    REG_I2CDATA = reg;
    REG_I2CCNT = I2CCNT_ENABLE | I2CCNT_ENABLE_IRQ | flags;
    return i2cGetResult();
}

u8 aptI2cWrite(u8 device, u16 reg, u16 data)
{
    i2cSetDelay(device);

    for (int i = 0; i < 8; i++)
    {
        if (aptSelectDevice(device, I2C_START) && aptSelectRegister(reg >> 8, I2C_NONE)
            && aptSelectRegister(reg & 0xFF, I2C_NONE))
        {
            i2cDelay();
            if (aptSetData(data >> 8, I2C_NONE) && aptSetData(data & 0xFF, I2C_STOP))
                return 1;
        }
        REG_I2CCNT = I2CCNT_ENABLE | I2CCNT_ENABLE_IRQ | I2CCNT_ERROR | I2CCNT_STOP;
    }

    return 0;
}

u16 aptI2cRead(u8 device, u16 reg)
{
    i2cSetDelay(device);

    for (int i = 0; i < 8; i++)
    {
        if (aptSelectDevice(device, I2C_START) && aptSelectRegister(reg >> 8, I2C_NONE)
            && aptSelectRegister(reg & 0xFF, I2C_STOP))
        {
            i2cDelay();
            if (aptSelectDevice(device | 1, I2C_START))
            {
                return (aptGetData(I2C_READ | I2C_ACK) << 8)
                       | aptGetData(I2C_STOP | I2C_READ);
            }
        }

        REG_I2CCNT = I2CCNT_ENABLE | I2CCNT_ENABLE_IRQ | I2CCNT_ERROR | I2CCNT_STOP;
    }

    return 0xFFFF;
}
