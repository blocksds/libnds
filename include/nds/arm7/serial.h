// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2005-2011 Michael Noland (joat)
// Copyright (C) 2005-2011 Jason Rogers (dovoto)
// Copyright (C) 2005-2011 Dave Murphy (WinterMute)

// SPI control for the ARM7

#ifndef LIBNDS_NDS_ARM7_SERIAL_H__
#define LIBNDS_NDS_ARM7_SERIAL_H__

#ifdef __cplusplus
extern "C" {
#endif

#ifndef ARM7
#error Serial header is for ARM7 only
#endif

/// @file nds/arm7/serial.h
///
/// @brief SPI bus controller ARM7 helpers.

#include <nds/bios.h>
#include <nds/input.h>

// 'Networking'
#define REG_RCNT                (*(vu16 *)0x04000134)
#define REG_KEYXY               (*(vu16 *)0x04000136)

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

// SPI target: device + frequency
#define SPI_TARGET_POWER    (SPI_DEVICE_POWER    | SPI_BAUD_1MHz)
#define SPI_TARGET_FIRMWARE (SPI_DEVICE_FIRMWARE | SPI_BAUD_4MHz)
#define SPI_TARGET_TSC      (SPI_DEVICE_TOUCH    | SPI_BAUD_2MHz)
#define SPI_TARGET_CODEC    (SPI_DEVICE_TOUCH    | SPI_BAUD_4MHz)

// When used, the /CS line will stay low after the transfer ends
// i.e. when we're part of a continuous transfer
#define SPI_CONTINUOUS          BIT(11)

/// Wait until the SPI bus is available.
void spiWaitBusy(void);

#define SerialWaitBusy spiWaitBusy

/// Does an exchange in the SPI bus (sends a value while a value is received).
///
/// @param value
///     Value to write to the SPI bus.
///
/// @return
///     Returns the value read from the SPI bus.
u8 spiExchange(u8 value);

/// Writes a value to the SPI bus.
///
/// @param value
///     Value to write to the SPI bus.
void spiWrite(u8 value);

/// Reads a value from the SPI bus (by doing an exchange and sending a 0).
///
/// @return
///     Returns the value read from the SPI bus.
u8 spiRead(void);

#ifdef __cplusplus
}
#endif

#endif // LIBNDS_NDS_ARM7_SERIAL_H__
