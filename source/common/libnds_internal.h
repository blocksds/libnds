// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2005-2008 Dave Murphy (WinterMute)
// Copyright (c) 2023 Antonio Niño Díaz

// Internal variables for libnds

#ifndef COMMON_LIBNDS_INTERNAL_H__
#define COMMON_LIBNDS_INTERNAL_H__

#include <time.h>

#include <nds/arm9/input.h>
#include <nds/ndstypes.h>
#include <nds/system.h>

typedef struct __TransferRegion
{
    vs16 touchX, touchY;     // TSC X, Y
    vs16 touchXpx, touchYpx; // TSC X, Y pixel values
    vs16 touchZ1, touchZ2;   // TSC x-panel measurements
    vu16 buttons;            // X, Y, /PENIRQ buttons
    time_t unixTime;
    struct __bootstub *bootcode;
} __TransferRegion, *__pTransferRegion;

#define transfer (*(__TransferRegion volatile *)(0x02FFF000))

static inline void setTransferInputData(touchPosition *touch, u16 buttons)
{
    transfer.buttons = buttons;
    transfer.touchX = touch->rawx;
    transfer.touchY = touch->rawy;
    transfer.touchXpx = touch->px;
    transfer.touchYpx = touch->py;
    transfer.touchZ1 = touch->z1;
    transfer.touchZ2 = touch->z2;
}

static inline __TransferRegion volatile *__transferRegion(void)
{
    return &transfer;
}

void __libnds_exit(int rc);

extern time_t *punixTime;

#endif // COMMON_LIBNDS_INTERNAL_H__
