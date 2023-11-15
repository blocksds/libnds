// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2020 Gericom
// Copyright (C) 2023 Antonio Niño Díaz

#include <nds/asminc.h>

BEGIN_ASM_FUNC dspSpinWait itcm

    // The jump to here took at least 3 cycles. The jump back took at least 3
    // cycles as well. 2 nops should do.
    nop
    nop
    bx lr
