// SPDX-License-Identifier: Zlib
//
// Copyright (c) 2023 Antonio Niño Díaz

#include <nds/asminc.h>

    .text
    .arm

BEGIN_ASM_FUNC getCPSR

    mrs     r0, cpsr
    bx      lr

BEGIN_ASM_FUNC setCPSR

    msr     cpsr, r0
    bx      lr
