// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2024 Dominik Kurz

#include <nds/asminc.h>

.syntax  unified
.arm
.text
BEGIN_ASM_FUNC start_div64_64
    mov r12, #0x04000000;
    add r12, r12, #0x290;
    stmia r12, {r0-r3};     @store {r0-r1} to REG_DIV_NUMER,{r2,r3} to REG_DIV_DENOM
    ldr r0, [r12, #-0x10];  @load REG_DIV_CNT
    mov r1, #2;
    and r0, r0, #3;         @mask mode with DIV_MODE_MASK
    cmp r0, #2;             @compare mode to DIV_64_64
    strne r1, [r12, #-0x10];@set REG_DIV_CNT to DIV_64_64
    bx lr;
.arm
.text
BEGIN_ASM_FUNC finish_div64_64
    mov r1, #0x04000000;
    add r1, r1, #0x290;
    ldr r0, [r1, #-0x10];   @load REG_DIV_CNT
1:
    tst r0, #0x8000;        @check whether result is ready
    ldrne r0, [r1, #-0x10]; @load REG_DIV_CNT
    bne 1b;
    ldrd r0, r1,[r1, #0x10];@load result from REG_DIV_RESULT
    bx lr
