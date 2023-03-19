// SPDX-License-Identifier: Zlib
//
// Copyright (c) 2023 Antonio Niño Díaz

#ifndef NDS_ARM9_CARD_H__
#define NDS_ARM9_CARD_H__

#include <stdbool.h>
#include <stddef.h>

// Function to ask the ARM7 to read from the slot-1 using card commands
bool cardReadArm7(void *dest, size_t offset, size_t size);

#endif // NDS_ARM9_CARD_H__
