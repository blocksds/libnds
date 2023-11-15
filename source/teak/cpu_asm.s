// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2020 Gericom
// Copyright (C) 2023 Antonio Niño Díaz

#include <teak/asminc.h>

BEGIN_ASM_FUNC cpuDisableIrqs

    dint
    ret     always

BEGIN_ASM_FUNC cpuEnableIrqs

    eint
    ret     always

BEGIN_ASM_FUNC cpuDisableInt0

    rst     0x100, mod3
    ret     always

BEGIN_ASM_FUNC cpuEnableInt0

    set     0x100, mod3
    ret     always

BEGIN_ASM_FUNC cpuDisableInt1

    rst     0x200, mod3
    ret     always

BEGIN_ASM_FUNC cpuEnableInt1

    set     0x200, mod3
    ret     always

BEGIN_ASM_FUNC cpuDisableInt2

    rst     0x400, mod3
    ret     always

BEGIN_ASM_FUNC cpuEnableInt2

    set     0x400, mod3
    ret     always

BEGIN_ASM_FUNC cpuDisableVInt

    rst     0x800, mod3
    ret     always

BEGIN_ASM_FUNC cpuEnableVInt

    set     0x800, mod3
    ret     always
