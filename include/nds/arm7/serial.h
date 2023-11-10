// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2005-2011 Michael Noland (joat)
// Copyright (C) 2005-2011 Jason Rogers (dovoto)
// Copyright (C) 2005-2011 Dave Murphy (WinterMute)

// SPI control for the ARM7

#ifndef LIBNDS_NDS_ARM7_SERIAL_H__
#define LIBNDS_NDS_ARM7_SERIAL_H__

#ifndef ARM7
#error Serial header is for ARM7 only
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include <nds/bios.h>
#include <nds/input.h>

// 'Networking'
#define REG_RCNT                (*(vu16 *)0x04000134)
#define REG_KEYXY               (*(vu16 *)0x04000136)
#define RTC_CR                  (*(vu16 *)0x04000138)
#define RTC_CR8                 (*(vu8 *)0x04000138)

#define REG_SIOCNT              (*(vu16 *)0x04000128)

#define SIO_DATA8               (*(vu8 *)0x0400012A)
#define SIO_DATA32              (*(vu32 *)0x04000120)

// FIXME: Does the hardware still support 16 bit comms mode?
// BIOS makes use of 32 bit mode, so some regs still exist
#define SIO_MULTI_0             (*(vu16 *)0x04000120)
#define SIO_MULTI_1             (*(vu16 *)0x04000122)
#define SIO_MULTI_2             (*(vu16 *)0x04000124)
#define SIO_MULTI_3             (*(vu16 *)0x04000126)
#define SIO_MULTI_SEND          (*(vu16 *)0x0400012A)


// SPI chain registers
#define REG_SPICNT              (*(vu16 *)0x040001C0)
#define REG_SPIDATA             (*(vu16 *)0x040001C2)

#define SPI_ENABLE              BIT(15)
#define SPI_IRQ                 BIT(14)
#define SPI_BUSY                BIT(7)

// Supported speeds
#define SPI_BAUD_4MHz           0
#define SPI_BAUD_2MHz           1
#define SPI_BAUD_1MHz           2
#define SPI_BAUD_512KHz         3

// Pick the SPI transfer length
#define SPI_BYTE_MODE           (0 << 10)
#define SPI_HWORD_MODE          (1 << 10)

// Pick the SPI device
#define SPI_DEVICE_POWER        (0 << 8)
#define SPI_DEVICE_FIRMWARE     (1 << 8)
#define SPI_DEVICE_NVRAM        (1 << 8)
#define SPI_DEVICE_TOUCH        (2 << 8)
#define SPI_DEVICE_MICROPHONE   (2 << 8)

// When used, the /CS line will stay low after the transfer ends
// i.e. when we're part of a continuous transfer
#define SPI_CONTINUOUS          BIT(11)

// FIXME: does this stuff really belong in serial.h?

// Firmware commands
#define FIRMWARE_WREN           0x06
#define FIRMWARE_WRDI           0x04
#define FIRMWARE_RDID           0x9F
#define FIRMWARE_RDSR           0x05
#define FIRMWARE_READ           0x03
#define FIRMWARE_PW             0x0A
#define FIRMWARE_PP             0x02
#define FIRMWARE_FAST           0x0B
#define FIRMWARE_PE             0xDB
#define FIRMWARE_SE             0xD8
#define FIRMWARE_DP             0xB9
#define FIRMWARE_RDP            0xAB

static inline void SerialWaitBusy(void)
{
    while (REG_SPICNT & SPI_BUSY);
}

// Read the firmware
void readFirmware(u32 address, void *destination, u32 size);

// Read internal flash JEDEC values
int readJEDEC(u8 *destination, u32 size);

#ifdef __cplusplus
}
#endif

#endif // LIBNDS_NDS_ARM7_SERIAL_H__
