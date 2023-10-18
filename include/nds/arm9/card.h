// SPDX-License-Identifier: Zlib
//
// Copyright (c) 2023 Antonio Niño Díaz

#ifndef LIBNDS_NDS_ARM9_CARD_H__
#define LIBNDS_NDS_ARM9_CARD_H__

/// @file nds/arm9/card.h
///
/// @brief Slot-1 read functions.

#include <stdbool.h>
#include <stddef.h>

/// Function that asks the ARM7 to read from the slot-1 using card commands.
///
/// @param dest Destination buffer.
/// @param offset NDS ROM offset to read.
/// @param size Size in bytes to read.
/// @return On error it returns true. On success, it returns false.
bool cardReadArm7(void *dest, size_t offset, size_t size);

#endif // LIBNDS_NDS_ARM9_CARD_H__
