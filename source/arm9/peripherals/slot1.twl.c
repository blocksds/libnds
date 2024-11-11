// SPDX-License-Identifier: Zlib
//
// Copyright (c) 2024 Adrian "asie" Siekierka

// Slot-1 helper routines

#include <nds.h>

void libnds_cardInitTWL(void)
{
    // DSi homebrew environments do not guarantee initializing the
    // Slot-1 cartridge. Do so manually.
    // TODO: This could probably be optimized.
    disableSlot1();
    enableSlot1();
    for (int i = 0; i < 20; i++)
        swiWaitForVBlank();

    uint8_t header[512];
    cardReadHeader(header);
}
