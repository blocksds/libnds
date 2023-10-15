// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2023 Gericom
#if 0

#ifdef LIBTWL_ARM9
.section ".itcm", "ax"
#else
.text
#endif
.arm

//r0=dma number
//r1=src
//r2=dst
//r3=control
.global dma_ntrSetParams
dma_ntrSetParams:
    ldr r12,= 0x040000B0
    add r0, r0, r0, lsl #1 //multiply by 3 (regs)
    add r12, r12, r0, lsl #2
    stmia r12, {r1,r2,r3}
    b 1f //delay for safety
1:
    bx lr

//r0=dma number
.global dma_ntrStopSafe
dma_ntrStopSafe:
    //disable irqs
    mrs r3, cpsr
    orr r1, r3, #0x80
    msr cpsr_c, r1

    ldr r12,= 0x040000BA
    add r0, r0, r0, lsl #1 //multiply by 3 (regs)
    add r12, r12, r0, lsl #2
    ldrh r0, [r12]
    bic r0, r0, #0x3A00 //clear mode and repeat bits
    strh r0, [r12]
    //delay for safety
    b 1f
1:
#ifdef LIBTWL_ARM9
    b 2f
2:
#endif
    ldrh r0, [r12]
    bic r0, r0, #0x8000 //clear enable bit
    strh r0, [r12]

    //restore irqs
    msr cpsr_c, r3

    bx lr

.pool
#endif
.end
