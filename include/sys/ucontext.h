// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2021-2023 agbabi contributors

// Context switching definitions

#ifndef SYS_UCONTEXT_H__
#define SYS_UCONTEXT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <signal.h>

typedef struct
{
    unsigned int reg_r0;
    unsigned int reg_r1;
    unsigned int reg_r2;
    unsigned int reg_r3;
    unsigned int reg_r4;
    unsigned int reg_r5;
    unsigned int reg_r6;
    unsigned int reg_r7;
    unsigned int reg_r8;
    unsigned int reg_r9;
    unsigned int reg_r10;
    unsigned int reg_r11;
    unsigned int reg_r12;
    unsigned int reg_sp;
    unsigned int reg_lr;
    unsigned int reg_pc;
    unsigned int reg_cpsr;
} mcontext_t;

typedef struct ucontext_t
{
    struct ucontext_t *uc_link;
    stack_t uc_stack;
    mcontext_t uc_mcontext;
} ucontext_t;

#ifdef __cplusplus
}
#endif

#endif // SYS_UCONTEXT_H__
