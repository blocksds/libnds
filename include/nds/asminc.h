// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2017-2018 Dave Murphy (WinterMute)

#ifndef LIBNDS_ASMINC_H__
#define LIBNDS_ASMINC_H__

#if !__ASSEMBLER__
# error "This header file is only for use in assembly files!"
#endif

.macro BEGIN_ASM_FUNC name section=text
    .section .\section\().\name\(), "ax", %progbits
    .global \name
    .type \name, %function
    .align 2
\name:
.endm

#define ICACHE_SIZE     0x2000
#define DCACHE_SIZE     0x1000
#define CACHE_LINE_SIZE 32

#endif // LIBNDS_ASMINC_H__
