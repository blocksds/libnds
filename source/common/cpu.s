// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2023-2026 Antonio Niño Díaz

#include <nds/asminc.h>
#include <nds/cpu_asm.h>

    .syntax  unified
    .text
    .arm

BEGIN_ASM_FUNC getCPSR

    mrs     r0, cpsr
    bx      lr

BEGIN_ASM_FUNC setCPSR

    msr     cpsr, r0
    bx      lr

BEGIN_ASM_FUNC cpsrEnableInterrupts itcm

    mrs     r0, cpsr
    bic     r0, r0, #CPSR_FLAG_IRQ_DIS
    msr     cpsr, r0

    bx      lr

BEGIN_ASM_FUNC cpsrDisableInterrupts itcm

    mrs     r0, cpsr
    orr     r0, r0, #CPSR_FLAG_IRQ_DIS
    msr     cpsr, r0

    bx      lr

BEGIN_ASM_FUNC cpsrCriticalSectionStart itcm

    mrs     r0, cpsr
    orr     r1, r0, #CPSR_FLAG_IRQ_DIS
    msr     cpsr, r1

    // Return old cpsr
    bx      lr

BEGIN_ASM_FUNC cpsrCriticalSectionEnd itcm

    and     r0, r0, #CPSR_FLAG_IRQ_DIS

    mrs     r1, cpsr
    bic     r1, r1, #CPSR_FLAG_IRQ_DIS

    orr     r0, r0, r1
    msr     cpsr, r0

    bx      lr
