// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2009 Dave Murphy (WinterMute)
// Copyright (C) 2023 Antonio Niño Díaz

#include <nds/asminc.h>

#ifdef ARM9
#include <nds/arm9/cp15_asm.h>

    .arch   armv5te
    .cpu    arm946e-s
#endif

    .text
    .align 4
    .arm

BEGIN_ASM_FUNC swiSoftReset

    // Disable interrupts (REG_IME = 0)
    mov     r0, #0x4000000
    str     r0, [r0, #0x208]

#ifdef ARM7

    ldr     r0, =0x2FFFE34
    ldr     r0, [r0]
    bx      r0

#endif

#ifdef ARM9

    // Disable TCM, caches and protection unit
    ldr     r0, =(CP15_CONTROL_ALTERNATE_VECTOR_SELECT | CP15_CONTROL_RESERVED_SBO_MASK)
    mcr     CP15_REG1_CONTROL_REGISTER(r0)

    // Disable cache
    mov     r0, #0
    mcr     CP15_REG7_FLUSH_ICACHE
    mcr     CP15_REG7_FLUSH_DCACHE

    // Wait for write buffer to empty
    mcr     CP15_REG7_DRAIN_WRITE_BUFFER

    ldr     r0, =0x2FFFE24
    ldr     r0, [r0]
    bx      r0

#endif

    .pool
