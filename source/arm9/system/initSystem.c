// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2007-2021 Dave Murphy (WinterMute)

// Code for initialising the DS

#include <time.h>

#include <nds/arm9/exceptions.h>
#include <nds/arm9/input.h>
#include <nds/arm9/sprite.h>
#include <nds/arm9/video.h>
#include <nds/dma.h>
#include <nds/fifocommon.h>
#include <nds/interrupts.h>
#include <nds/ipc.h>
#include <nds/memory.h>
#include <nds/ndstypes.h>
#include <nds/system.h>
#include <nds/timers.h>

#include "common/libnds_internal.h"

bool __dsimode; // Set by the crt0
bool __debugger_unit; // Set by the crt0

time_t *punixTime;

// Reset the DS registers to sensible defaults
void __attribute__((weak)) initSystem(void)
{
    // Stop timers and dma
    for (int i = 0; i < 4; i++)
    {
        DMA_CR(i) = 0;
        DMA_SRC(i) = 0;
        DMA_DEST(i) = 0;
        TIMER_CR(i) = 0;
        TIMER_DATA(i) = 0;
    }

    if (__debugger_unit)
    {
        // Debugger units come with a system monitor in the last 512 KB of RAM.
        // If the developer wants to use them it's required to call
        // reduceHeapSize(0) in the application code.
        reduceHeapSize(512 * 1024);
        // TODO: This size is confirmed for DS units, but we need to check DSi
        // debugger units, it may be different.
    }
    else
    {
        // Setup an exception handler by default but not in debugger units.
        // Debugger units are very rare, they are used to develop applications,
        // and they come with their own exception handler. That means that users
        // of debugger units will know how to handle exceptions.
        //
        // For non-debugger models it's a good idea to setup an exception
        // handler by default because many developers will forget to do it by
        // themselves. The release exception handler only prints an error
        // message to reduce the code footprint of libnds. The debug exception
        // handler prints a lot more information.
        //
        // Release builds can also use the debug exception handler if
        // defaultExceptionHandler() is called from the application code.
#ifdef NDEBUG
        releaseExceptionHandler();
#else
        defaultExceptionHandler();
#endif
    }

    // Clear video display registers
    dmaFillWords(0, (void *)0x04000000, 0x58);
    dmaFillWords(0, (void *)0x04001008, 0x58 - 8);

    // Turn on power for 2D video
    REG_POWERCNT = (POWER_LCD | POWER_2D_A | POWER_2D_B | POWER_SWAP_LCDS) & 0xFFFF;

    videoSetModeSub(0);

    vramDefault();

    if (isDSiMode())
        setCpuClock(true);

    irqInit();
    fifoInit();

    fifoSetValue32Handler(FIFO_SYSTEM, systemValueHandler, 0);
    fifoSetDatamsgHandler(FIFO_SYSTEM, systemMsgHandler, 0);

    punixTime = (time_t *)memUncached((void *)&__transferRegion()->unixTime);

    __transferRegion()->bootcode = __system_bootstub;
    irqEnable(IRQ_VBLANK);
}
