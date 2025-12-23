// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2023-2025 Antonio Niño Díaz

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
//
// Note: Using TLS from an interrupt handler is problematic. The code uses the
// TLS area of the thread that was interrupted, which may cause weird issues.
// Even when interrupts are nested, the same TLS area is used. In debug builds
// we need to check if this function is called from an interrupt handler to
// detect this situation. The check makes the function twice as slow, so it
// isn't a good idea to keep it in release builds for now.

    .syntax  unified
    .thumb

BEGIN_ASM_FUNC __aeabi_read_tp

#ifndef NDEBUG
    ldr     r0, =irq_nesting_level
    ldr     r0, [r0]
    cmp     r0, #0
    beq     not_in_irq

    // Jump to libndsCrash(), which should never return
    ldr     r0, =__aeabi_read_tp_error_msg
    ldr     r1, =libndsCrash
    bx      r1

    // libndsCrash() never returns, it's safe to place the message here
__aeabi_read_tp_error_msg:
    .asciz "Using TLS from IRQ"
    .balign 2

not_in_irq:
#endif

    ldr     r0, =__tls
    ldr     r0, [r0]
    bx      lr
