// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2020 Gericom

.text

.global cpuDisableIrqs
cpuDisableIrqs:
    dint
    ret always

.global cpuEnableIrqs
cpuEnableIrqs:
    eint
    ret always

.global cpuDisableInt0
cpuDisableInt0:
    rst 0x100, mod3
    ret always

.global cpuEnableInt0
cpuEnableInt0:
    set 0x100, mod3
    ret always

.global cpuDisableInt1
cpuDisableInt1:
    rst 0x200, mod3
    ret always

.global cpuEnableInt1
cpuEnableInt1:
    set 0x200, mod3
    ret always

.global cpuDisableInt2
cpuDisableInt2:
    rst 0x400, mod3
    ret always

.global cpuEnableInt2
cpuEnableInt2:
    set 0x400, mod3
    ret always

.global cpuDisableVInt
cpuDisableVInt:
    rst 0x800, mod3
    ret always

.global cpuEnableVInt
cpuEnableVInt:
    set 0x800, mod3
    ret always
