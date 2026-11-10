// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2023 Antonio Niño Díaz

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
    bic     r0, #CPSR_FLAG_IRQ_DIS
    msr     cpsr, r0

    bx      lr

BEGIN_ASM_FUNC cpsrDisableInterrupts itcm

    mrs     r0, cpsr
    orr     r0, #CPSR_FLAG_IRQ_DIS
    msr     cpsr, r0

    bx      lr
