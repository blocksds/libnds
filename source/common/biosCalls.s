// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2005 Michael Noland (joat)
// Copyright (C) 2005 Jason Rogers (Dovoto)
// Copyright (C) 2005 Dave Murphy (WinterMute)

#include <nds/asminc.h>

    .syntax  unified

    .text
    .align 4

    .thumb

BEGIN_ASM_FUNC swiDelay

    swi     0x03
    bx      lr

BEGIN_ASM_FUNC swiSleep

    swi     0x07
    bx      lr

BEGIN_ASM_FUNC swiChangeSoundBias

    swi     0x08
    bx      lr

BEGIN_ASM_FUNC swiDivide

    swi     0x09
    bx      lr

BEGIN_ASM_FUNC swiRemainder

    swi     0x09
    movs    r0, r1
    bx      lr

BEGIN_ASM_FUNC swiDivMod

    push    {r2, r3}
    swi     0x09
    pop     {r2, r3}
    str     r0, [r2]
    str     r1, [r3]
    bx      lr

BEGIN_ASM_FUNC swiCopy

    swi     0x0B
    bx      lr

BEGIN_ASM_FUNC swiFastCopy

    swi     0x0C
    bx      lr

BEGIN_ASM_FUNC swiSqrt

    swi     0x0D
    bx      lr

BEGIN_ASM_FUNC swiCRC16

    swi     0x0E
    bx      lr

BEGIN_ASM_FUNC swiIsDebugger

    swi     0x0F
    bx      lr

BEGIN_ASM_FUNC swiUnpackBits

    swi     0x10
    bx      lr

BEGIN_ASM_FUNC swiDecompressLZSSWram

    swi     0x11
    bx      lr

BEGIN_ASM_FUNC swiDecompressLZSSVramNTR

    swi     0x12
    bx      lr

BEGIN_ASM_FUNC swiDecompressLZSSVramTWL

    swi     0x02
    bx      lr

BEGIN_ASM_FUNC swiDecompressHuffman

    swi     0x13
    bx      lr

BEGIN_ASM_FUNC swiDecompressRLEWram

    swi     0x14
    bx      lr

BEGIN_ASM_FUNC swiDecompressRLEVram

    swi     0x15
    bx      lr

// ARM7 only bios calls
// --------------------

#ifdef ARM7

BEGIN_ASM_FUNC swiHalt

    swi     0x06
    bx      lr

BEGIN_ASM_FUNC swiGetSineTable

    swi     0x1A
    bx      lr

BEGIN_ASM_FUNC swiGetPitchTable

    swi     0x1B
    bx      lr


BEGIN_ASM_FUNC swiGetVolumeTable

    swi     0x1C
    bx      lr

// ARM7 function, but no real point in exposing it, at least not
// without adding a way to get the 3 arguments back into C
//
// BEGIN_ASM_FUNC swiGetFptrs
//
//    swi     0x1D
//    bx      lr

BEGIN_ASM_FUNC swiSwitchToGBAMode

    movs    r0, #0x40
    swi     0x1F
    // Does not return, of course

BEGIN_ASM_FUNC swiSetHaltCR

    movs    r2, r0
    swi     0x1F
    bx      lr

#endif // ARM7

// ARM9 only bios calls
// --------------------

#ifdef ARM9

BEGIN_ASM_FUNC swiWaitForIRQ

    swi     0x06
    bx      lr

BEGIN_ASM_FUNC swiDecodeDelta8

    swi     0x16
    bx      lr

BEGIN_ASM_FUNC swiDecodeDelta16

    swi     0x18
    bx      lr

BEGIN_ASM_FUNC swiSetHaltCR

    swi     0x1F
    bx      lr

#endif // ARM9
