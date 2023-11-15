// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2020 Gericom
// Copyright (c) 2023 Antonio Niño Díaz

#ifndef LIBNDS_NDS_NWRAM_H__
#define LIBNDS_NDS_NWRAM_H__

#include <stdbool.h>

#include <nds/ndstypes.h>

/// @file nds/nwram.h
///
/// @brief New WRAM utilities.

/// Available NWRAM blocks.
typedef enum
{
    NWRAM_BLOCK_A = 0, ///< NWRAM block A
    NWRAM_BLOCK_B = 1, ///< NWRAM block B
    NWRAM_BLOCK_C = 2  ///< NWRAM block C
} NWRAM_BLOCK;

/// Possible image sizes of a NWRAM block
typedef enum
{
    NWRAM_BLOCK_IMAGE_SIZE_32K = 0, ///< 32 KB
    NWRAM_BLOCK_IMAGE_SIZE_64K,     ///< 64 KB
    NWRAM_BLOCK_IMAGE_SIZE_128K,    ///< 128 KB
    NWRAM_BLOCK_IMAGE_SIZE_256K,    ///< 256 KB
} NWRAM_BLOCK_IMAGE_SIZE;

/// Base address of NWRAM
#define NWRAM_BASE                  0x03000000

// NWRAM A
// =======

#define NWRAM_A_SLOT_SIZE           0x10000
#define NWRAM_A_SLOT_SHIFT          16
#define NWRAM_A_SLOT_COUNT          4

#define NWRAM_A_ADDRESS_MAX         0x03FF0000

#define NWRAM_A_SLOT_OFFSET(i)      ((i) << 2)
#define NWRAM_A_SLOT_ENABLE         0x80

/// Possible owners of NWRAM A slots
typedef enum
{
    NWRAM_A_SLOT_MASTER_ARM9 = 0, ///< The ARM9 is the owner
    NWRAM_A_SLOT_MASTER_ARM7 = 1  ///< The ARM7 is the owner
} NWRAM_A_SLOT_MASTER;

// NWRAM B and C
// =============

#define NWRAM_BC_SLOT_SIZE          0x8000
#define NWRAM_BC_SLOT_SHIFT         15
#define NWRAM_BC_SLOT_COUNT         8

#define NWRAM_BC_ADDRESS_MAX        0x03FF8000

#define NWRAM_BC_SLOT_OFFSET(i)     ((i) << 2)
#define NWRAM_BC_SLOT_ENABLE        0x80

/// Possible owners of NWRAM C slots
typedef enum
{
    NWRAM_B_SLOT_MASTER_ARM9 = 0,    ///< The ARM9 is the owner
    NWRAM_B_SLOT_MASTER_ARM7 = 1,    ///< The ARM7 is the owner
    NWRAM_B_SLOT_MASTER_DSP_CODE = 2 ///< The DSP is the owner. Used for code.
} NWRAM_B_SLOT_MASTER;

/// Possible owners of NWRAM C slots
typedef enum
{
    NWRAM_C_SLOT_MASTER_ARM9 = 0,    ///< The ARM9 is the owner
    NWRAM_C_SLOT_MASTER_ARM7 = 1,    ///< The ARM7 is the owner
    NWRAM_C_SLOT_MASTER_DSP_DATA = 2 ///< The DSP is the owner. Used for data.
} NWRAM_C_SLOT_MASTER;

/// Returns the address of a NWRAM block that has been mapped to a CPU.
///
/// @param block One of NWRAM_BLOCK.
/// @return The address.
u32 nwramGetBlockAddress(NWRAM_BLOCK block);

/// Maps a NWRAM block to a CPU to the specified address and length.
///
/// @param block One of NWRAM_BLOCK.
/// @param start The base address. Only 0x3000000 to 0x3800000 available.
/// @param length Length of the block.
/// @param imageSize Size of the block.
void nwramSetBlockMapping(NWRAM_BLOCK block, u32 start, u32 length,
                          NWRAM_BLOCK_IMAGE_SIZE imageSize);

#ifdef ARM9

/// Maps a slot of WRAM slot A to the specified CPU.
///
/// @param slot Slot index (0 to 3).
/// @param master Owner of the slot (ARM7, ARM9 or DSP).
/// @param offset Offset of the slot.
/// @param enable true to enable the slot, false to disable it.
void nwramMapWramASlot(int slot, NWRAM_A_SLOT_MASTER master, int offset, bool enable);

/// Maps a slot of WRAM slot B to the specified CPU.
///
/// @param slot Slot index (0 to 3).
/// @param master Owner of the slot (ARM7, ARM9 or DSP).
/// @param offset Offset of the slot.
/// @param enable true to enable the slot, false to disable it.
void nwramMapWramBSlot(int slot, NWRAM_B_SLOT_MASTER master, int offset, bool enable);

/// Maps a slot of WRAM slot C to the specified CPU.
///
/// @param slot Slot index (0 to 3).
/// @param master Owner of the slot (ARM7, ARM9 or DSP).
/// @param offset Offset of the slot.
/// @param enable true to enable the slot, false to disable it.
void nwramMapWramCSlot(int slot, NWRAM_C_SLOT_MASTER master, int offset, bool enable);

#endif // ARM9

#endif // LIBNDS_NDS_NWRAM_H__
