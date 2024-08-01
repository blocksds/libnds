// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2017 Dave Murphy (WinterMute)

#ifndef LIBNDS_ARM9_DLDI_ASM_H__
#define LIBNDS_ARM9_DLDI_ASM_H__

#define FEATURE_MEDIUM_CANREAD      0x00000001 ///< This driver can be used to read sectors.
#define FEATURE_MEDIUM_CANWRITE     0x00000002 ///< This driver can be used to write sectors.
#define FEATURE_SLOT_GBA            0x00000010 ///< This driver uses Slot-2 cartridges.
#define FEATURE_SLOT_NDS            0x00000020 ///< This driver uses Slot-1 cartridges.
#define FEATURE_ARM7_CAPABLE        0x00000100 ///< This driver can be safely used from ARM7 and ARM9. BlocksDS extension.

/// Addresses are to be fixed in the full text/data area of the DLDI driver.
///
/// As this may affect any opcodes and constants in the area, this is not
/// recommended.
#define FIX_ALL         0x01

/// Addresses are to be fixed in the interwork area of the DLDI driver.
#define FIX_GLUE        0x02

/// Addresses are to be fixed in the global offset area of the DLDI driver.
#define FIX_GOT         0x04

/// The driver's BSS area should be zero-filled on load.
#define FIX_BSS         0x08

#define DLDI_SIZE_32KB  0x0f
#define DLDI_SIZE_16KB  0x0e
#define DLDI_SIZE_8KB   0x0d
#define DLDI_SIZE_4KB   0x0c
#define DLDI_SIZE_2KB   0x0b
#define DLDI_SIZE_1KB   0x0a
#define DLDI_SIZE_512B  0x09

#endif // LIBNDS_ARM9_DLDI_ASM_H__
