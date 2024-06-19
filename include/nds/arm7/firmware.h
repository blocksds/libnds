// SPDX-License-Identifier: Zlib
//
// Copyright (c) 2024 Adrian "asie" Siekierka

#ifndef LIBNDS_NDS_ARM7_FIRMWARE_H__
#define LIBNDS_NDS_ARM7_FIRMWARE_H__

#ifdef __cplusplus
extern "C" {
#endif

#ifndef ARM7
#error Firmware header is for ARM7 only
#endif

/// @file nds/arm7/firmware.h
///
/// @brief DS firmware flash ARM7 helpers.

#include <nds/ndstypes.h>
#include <nds/system.h>

// Firmware commands
#define FIRMWARE_WREN           0x06 ///< Write Enable
#define FIRMWARE_WRDI           0x04 ///< Write Disable
#define FIRMWARE_RDID           0x9F ///< Read JEDEC ID
#define FIRMWARE_RDSR           0x05 ///< Read Status Register
#define FIRMWARE_READ           0x03 ///< Read Data Bytes
#define FIRMWARE_PW             0x0A ///< Page Write
#define FIRMWARE_PP             0x02 ///< Page Program
#define FIRMWARE_FAST           0x0B ///< Fast Read Data Bytes (preceded by a dummy byte)
#define FIRMWARE_PE             0xDB ///< Page Erase
#define FIRMWARE_SE             0xD8 ///< Sector Erase
#define FIRMWARE_DP             0xB9 ///< Deep Power Down
#define FIRMWARE_RDP            0xAB ///< Release from Deep Power Down

/**
 * @brief Read the JEDEC ID of the firmware flash.
 */
int readFirmwareJEDEC(u8 *destination, u32 size);

#ifdef __cplusplus
}
#endif

#endif
