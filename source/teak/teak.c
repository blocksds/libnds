// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2023 Antonio Niño Díaz

#include <teak/teak.h>

void teakInit(void)
{
    icuInit();

    // Applications wait for all bits to be 0 before connecting AHBM to DMA.
    while (ahbmIsBusy());

    dmaInit();
}
