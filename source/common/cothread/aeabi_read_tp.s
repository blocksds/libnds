// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2023 Antonio Niño Díaz

#include <nds/asminc.h>

// __aeabi_read_tp() is used by GCC to get a pointer to the thread local
// storage.
//
// This is a special function that can't clobber any registers other than r0 and
// lr. This is done so that calls to this function are very fast.
//
// For the most part, this function is used from user code, which is normally
// written in Thumb. Also, there is no point in placing this code in ITCM
// because any code located in main RAM will need to call a veneer to jump to
// this routine. It's better to place it in regular RAM.

    .syntax  unified
    .thumb

BEGIN_ASM_FUNC __aeabi_read_tp
    ldr     r0, =__tls
    ldr     r0, [r0]
    bx      lr
