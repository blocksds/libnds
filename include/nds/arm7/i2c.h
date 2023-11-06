// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2011 Dave Murphy (WinterMute)

// I2C control for the ARM7

#ifndef LIBNDS_NDS_ARM7_I2C_H__
#define LIBNDS_NDS_ARM7_I2C_H__

#ifndef ARM7
#error i2c header is for ARM7 only
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include <nds/ndstypes.h>

#define REG_I2CDATA     (*(vu8 *)0x4004500)
#define REG_I2CCNT      (*(vu8 *)0x4004501)

#define I2CCNT_STOP       BIT(0)
#define I2CCNT_START      BIT(1)
#define I2CCNT_ERROR      BIT(2)
#define I2CCNT_ACK        BIT(4)
#define I2CCNT_WRITE      (0)
#define I2CCNT_READ       BIT(5)
#define I2CCNT_ENABLE_IRQ BIT(6)
#define I2CCNT_ENABLE     BIT(7)
#define I2CCNT_BUSY       BIT(7)

static inline void i2cWaitBusy(void)
{
    while (REG_I2CCNT & I2CCNT_BUSY);
}

enum i2cDevices {
    I2C_CAM0    = 0x7A,
    I2C_CAM1    = 0x78,
    I2C_UNK1    = 0xA0,
    I2C_UNK2    = 0xE0,
    I2C_PM      = 0x4A,
    I2C_UNK3    = 0x40,
    I2C_GPIO    = 0x90
};

// Registers for Power Management (I2C_PM)
#define I2CREGPM_BATUNK         0x00
#define I2CREGPM_PWRIF          0x10
#define I2CREGPM_PWRCNT         0x11
#define I2CREGPM_MMCPWR         0x12
#define I2CREGPM_BATTERY        0x20
#define I2CREGPM_WIFILED        0x30
#define I2CREGPM_CAMLED         0x31
#define I2CREGPM_VOL            0x40
#define I2CREGPM_BACKLIGHT      0x41
#define I2CREGPM_RESETFLAG      0x70

u8 i2cWriteRegister(u8 device, u8 reg, u8 data);
u8 i2cReadRegister(u8 device, u8 reg);
u8 i2cWriteRegister16(u8 device, u16 reg, u16 data);
u16 i2cReadRegister16(u8 device, u16 reg);

#ifdef __cplusplus
}
#endif

#endif // LIBNDS_NDS_ARM7_I2C_H__
