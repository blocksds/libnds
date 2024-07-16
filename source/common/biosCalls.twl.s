// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2017 Dave Murphy (WinterMute)

#include <nds/asminc.h>

    .syntax  unified
    .text
    .align 4

    .thumb

BEGIN_ASM_FUNC swiRSAInitHeapTWL

    swi     0x20
    bx      lr

BEGIN_ASM_FUNC swiRSADecryptRAWTWL

    swi     0x21
    bx      lr

BEGIN_ASM_FUNC swiRSADecryptTWL

    swi     0x22
    bx      lr

BEGIN_ASM_FUNC swiRSADecryptPGPTWL

    swi     0x23
    bx      lr

BEGIN_ASM_FUNC swiSHA1InitTWL

    swi     0x24
    bx      lr

BEGIN_ASM_FUNC swiSHA1UpdateTWL

    swi     0x25
    bx      lr

BEGIN_ASM_FUNC swiSHA1FinalTWL

    swi     0x26
    bx      lr

BEGIN_ASM_FUNC swiSHA1CalcTWL

    swi     0x27
    bx      lr

BEGIN_ASM_FUNC swiSHA1VerifyTWL

    swi     0x28
    bx      lr
