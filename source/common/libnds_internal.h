// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2005-2008 Dave Murphy (WinterMute)
// Copyright (c) 2023 Antonio Niño Díaz

// Internal variables for libnds

#ifndef COMMON_LIBNDS_INTERNAL_H__
#define COMMON_LIBNDS_INTERNAL_H__

#include <stdio.h>
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

static inline __TransferRegion volatile *__transferRegion(void)
{
    // The transfer region address needs to be in an uncached mirror of main RAM
    // so that the code doesn't need to do any special cache handling when
    // trying to read updated values, or trying to ensure that the new value can
    // be read by the other CPU. The following regions are mapped in the MPU:
    //
    //            Cached main RAM            Uncached main RAM mirrors
    //
    // DS         0x2000000-0x2400000 (4M)   0x2400000-0x3000000 (12M) (3 times)
    // DS debug   0x2000000-0x2800000 (8M)   0x2800000-0x3000000 (8M)
    // DSi        0x2000000-0x3000000 (16M)  0xC000000-0xD000000 (16M)
    // DSi debug  0x2000000-0x3000000 (16M)  0xC000000-0xE000000 (32M)
    //
    // Also, it's important that the region isn't in DTCM, as it can't be seen
    // from the ARM7:
    //
    //            0x2FF0000-0x2FF4000 (16K)
    //
    // In DS mode, 0x2FFF000 is a good address, as it is inside an uncached main
    // RAM mirror, and outside DTCM. In a regular DSi, 0xCFFF000 is an
    // equivalent address.
    //
    // The only problem is the DSi debugger model. The main RAM of DSi at
    // 0xC000000 isn't mirrored at 0xD000000, so it isn't possible to use the
    // same address (let's say 0xDFFF000) for both the DSi (16 MB) and DSi
    // debugger (32 MB).
    //
    // This function could select different locations for each models but the
    // added complexity isn't worth it: The ARM9 linkerscript doesn't support
    // the additional 16 MB of the DSi debugger.
    if (isDSiMode())
        return (__TransferRegion volatile *)0x0CFFF000;
    else
        return (__TransferRegion volatile *)0x02FFF000;
}

static inline void setTransferInputData(touchPosition *touch, u16 buttons)
{
    __TransferRegion volatile *transfer = __transferRegion();

    transfer->buttons = buttons;
    transfer->touchX = touch->rawx;
    transfer->touchY = touch->rawy;
    transfer->touchXpx = touch->px;
    transfer->touchYpx = touch->py;
    transfer->touchZ1 = touch->z1;
    transfer->touchZ2 = touch->z2;
}

void __libnds_exit(int rc);

extern time_t *punixTime;

int nocash_putc_buffered(char c, FILE *file);
ssize_t nocash_write(const char *ptr, size_t len);

#endif // COMMON_LIBNDS_INTERNAL_H__
