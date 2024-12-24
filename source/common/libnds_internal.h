// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2005-2008 Dave Murphy (WinterMute)
// Copyright (c) 2023-2024 Antonio Niño Díaz

// Internal variables for libnds

#ifndef COMMON_LIBNDS_INTERNAL_H__
#define COMMON_LIBNDS_INTERNAL_H__

#include <assert.h>
#include <stdio.h>
#include <time.h>

#include <nds/ndstypes.h>
#include <nds/system.h>

// ARM7-ARM9 shared memory

typedef struct {
    uint32_t reg[16]; // State of user CPU registers
    uint32_t address; // Address that was accessed and caused the exception
    uint32_t stack[22]; // Dump of the stack at the SP
    char description[32]; // Reason for the exception
} ExceptionState;

typedef struct __TransferRegion
{
    time_t unixTime;
    struct __bootstub *bootcode;
    ExceptionState exceptionState;
} __TransferRegion, *__pTransferRegion;

static_assert(sizeof(__TransferRegion) <= 0x1000, "Transfer region is too big");

WARN_UNUSED_RESULT
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

// Exception-related functions

extern const char *exceptionMsg;

uint32_t ARMShift(uint32_t value, uint8_t shift);
u32 getExceptionAddress(u32 opcodeAddress, u32 thumbState);

void exceptionStatePrint(ExceptionState *ex, const char *title);

// Other functions present in the ARM7 and ARM9

void __libnds_exit(int rc);

int nocash_putc_buffered(char c, FILE *file);
ssize_t nocash_write(const char *ptr, size_t len);

// This function will cause an exception that will print the provided message.
__attribute__((noreturn)) void libndsCrash(const char *message);

#endif // COMMON_LIBNDS_INTERNAL_H__
