// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2021-2023 agbabi contributors
//
// Support:
//    __ndsabi_fiq_memcpy4, __ndsabi_fiq_memcpy4x4

#include <nds/asminc.h>

#include "macros.inc"

    .syntax unified

    .arm


BEGIN_ASM_FUNC __ndsabi_fiq_memcpy4

    cmp     r2, #48
    blt     .Lcopy_words

    push    {r4-r7}
    mrs     r3, cpsr

    @ Enter FIQ mode
    bic     r12, r3, #0x1f
    orr     r12, #0x11
    msr     cpsr, r12
    msr     spsr, r3

.Lloop_48:
    subs    r2, r2, #48
    ldmiage r1!, {r3-r14}
    stmiage r0!, {r3-r14}
    bgt     .Lloop_48

    @ Exit FIQ mode
    mrs     r3, spsr
    msr     cpsr, r3
    pop     {r4-r7}

    adds    r2, r2, #48
    bxeq    lr

.Lcopy_words:
    subs    r2, r2, #4
    ldrge   r3, [r1], #4
    strge   r3, [r0], #4
    bgt     .Lcopy_words
    bxeq    lr

    @ Copy byte & half tail
    joaobapt_test r2
    @ Copy half
    ldrhcs  r3, [r1], #2
    strhcs  r3, [r0], #2
    @ Copy byte
    ldrbmi  r3, [r1]
    strbmi  r3, [r0]
    bx      lr


BEGIN_ASM_FUNC __ndsabi_fiq_memcpy4x4

    push    {r4-r10}
    cmp     r2, #48
    blt     .Lcopy_tail_4x4

    @ Enter FIQ mode
    mrs     r3, cpsr
    bic     r12, r3, #0x1f
    orr     r12, #0x11
    msr     cpsr, r12
    msr     spsr, r3

.Lloop_48_4x4:
    subs    r2, r2, #48
    ldmiage r1!, {r3-r14}
    stmiage r0!, {r3-r14}
    bgt     .Lloop_48_4x4

    @ Exit FIQ mode
    mrs     r3, spsr
    msr     cpsr, r3

.Lcopy_tail_4x4:
    @ JoaoBapt test 48-bytes
    joaobapt_test_lsl r2, #27
    ldmiacs r1!, {r3-r10}
    stmiacs r0!, {r3-r10}
    ldmiami r1!, {r3-r6}
    stmiami r0!, {r3-r6}

    pop     {r4-r10}
    bx      lr
