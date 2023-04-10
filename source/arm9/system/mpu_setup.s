// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2009-2017 Dave Murphy (WinterMute)
// Copyright (C) 2017 fincs

#include <nds/asminc.h>
//#include <nds/arm9/cache_asm.h>
#include <nds/arm9/cp15_asm.h>

    .arch   armv5te
    .cpu    arm946e-s

    .text
    .arm

BEGIN_ASM_FUNC __libnds_mpu_setup

    // Turn on power for video mode 3
    // REG_POWCNT1 = POWER_LCD | POWER_2D_A | POWER_2D_B | POWER_SWAP_LCDS
    ldr     r1, =0x8203
    mov     r0, #0x04000000
    add     r0, r0, #0x304
    strh    r1, [r0]

    // Disable TCM and protection unit
    ldr     r1, =(CP15_CONTROL_ALTERNATE_VECTOR_SELECT | CP15_CONTROL_RESERVED_SBO_MASK)
    mcr     CP15_REG1_CONTROL_REGISTER(r1)

    // Protection Unit Setup added by Sasq

    // Disable cache
    mov     r0, #0
    mcr     CP15_REG7_FLUSH_ICACHE
    mcr     CP15_REG7_FLUSH_DCACHE

    // Wait for the write buffer to be empty
    mcr     CP15_REG7_DRAIN_WRITE_BUFFER

    ldr     r0, =__dtcm_start
    orr     r0, r0, #0x0a // TODO: BUG?
    mcr     CP15_REG9_DTCM_CONTROL(r0)  // DTCM base = __dtcm_start, size = 16 KB

    mov     r0, #0x20 // TODO
    mcr     CP15_REG9_ITCM_CONTROL(r0)  // ITCM base = 0, size = 32 MB

    // Setup memory regions similar to Release Version

    // Region 0 - IO registers
    ldr     r0, =(0x04000000 | CP15_REGION_SIZE_64MB | CP15_CONFIG_REGION_ENABLE)
    mcr     CP15_REG6_PROTECTION_REGION(r0, 0)

    // Region 1 - System ROM
    ldr     r0, =(0xFFFF0000 | CP15_REGION_SIZE_64KB | CP15_CONFIG_REGION_ENABLE)
    mcr     CP15_REG6_PROTECTION_REGION(r0, 1)

    // Region 2 - alternate vector base
    ldr     r0, =(0x00000000 | CP15_REGION_SIZE_4KB | CP15_CONFIG_REGION_ENABLE)
    mcr     CP15_REG6_PROTECTION_REGION(r0, 2)

    // Region 5 - DTCM
    ldr     r0, =__dtcm_start
    orr     r0, r0, #(CP15_REGION_SIZE_16KB | CP15_CONFIG_REGION_ENABLE)
    mcr     CP15_REG6_PROTECTION_REGION(r0, 5)

    // Region 4 - ITCM
    ldr     r0, =__itcm_start

    // Align to 32k
    mov     r0, r0, lsr #15
    mov     r0, r0, lsl #15

    orr     r0, r0, #(CP15_REGION_SIZE_32KB | CP15_CONFIG_REGION_ENABLE)
    mcr     CP15_REG6_PROTECTION_REGION(r0, 4)

    ldr     r0, =0x4004008 // SCFG_EXT9
    ldr     r0, [r0]
    // Bits 14-15: Main RAM Limit (0..1=4MB/DS, 2=16MB/DSi, 3=32MB/DSiDebugger)
    tst     r0, #0x8000
    bne     dsi_mode

    swi     0xf0000 // IsDebugger

    ldr     r1, =(0x08000000 | CP15_REGION_SIZE_128MB | CP15_CONFIG_REGION_ENABLE)
    cmp     r0, #0
    bne     debug_mode

    ldr     r3, =(0x02000000 | CP15_REGION_SIZE_4MB | CP15_CONFIG_REGION_ENABLE)
    ldr     r2, =(0x02000000 | CP15_REGION_SIZE_16MB | CP15_CONFIG_REGION_ENABLE)
    mov     r8, #0x02400000

    ldr     r9, =dsmasks
    b       setregions

debug_mode:
    ldr     r3, =(0x02000000 | CP15_REGION_SIZE_8MB | CP15_CONFIG_REGION_ENABLE)
    ldr     r2, =(0x02800000 | CP15_REGION_SIZE_8MB | CP15_CONFIG_REGION_ENABLE)
    mov     r8, #0x02800000
    ldr     r9, =debugmasks
    b       setregions

dsi_mode:
    tst     r0, #0x4000
    ldr     r1, =(0x03000000 | CP15_REGION_SIZE_8MB | CP15_CONFIG_REGION_ENABLE)
    ldr     r3, =(0x02000000 | CP15_REGION_SIZE_16MB | CP15_CONFIG_REGION_ENABLE)
    ldreq   r2, =(0x0C000000 | CP15_REGION_SIZE_16MB | CP15_CONFIG_REGION_ENABLE)
    // DSi debugger extended IWRAM
    ldrne   r2, =(0x0C000000 | CP15_REGION_SIZE_32MB | CP15_CONFIG_REGION_ENABLE)
    mov     r8, #0x03000000
    ldr     r9, =dsimasks

setregions:

    // Region 3 - DS Accessory (GBA Cart) / DSi switchable IWRAM
    mcr     CP15_REG6_PROTECTION_REGION(r1, 3)

    // Region 6 - Non-cacheable main RAM
    mcr     CP15_REG6_PROTECTION_REGION(r2, 6)

    // Region 7 - Cacheable main RAM
    mcr     CP15_REG6_PROTECTION_REGION(r3, 7)

    // Write buffer enable for region 7
    ldr     r0, =CP15_CONFIG_AREA_IS_BUFFERABLE(7)
    mcr     CP15_REG3_WRITE_BUFFER_CONTROL(r0)

    // Enable data and instruction caches for regions 1 and 7
    ldr     r0, =(CP15_CONFIG_AREA_IS_CACHABLE(1) | \
                  CP15_CONFIG_AREA_IS_CACHABLE(7))
    mcr     CP15_REG2_DATA_CACHE_CONFIG(r0)
    mcr     CP15_REG2_INSTRUCTION_CACHE_CONFIG(r0)

    // Instruction access permission (RW except for region 1, which is RO)
    ldr     r0, =(CP15_AREA_ACCESS_PERMISSIONS_PRW_URW(0) | \
                  CP15_AREA_ACCESS_PERMISSIONS_PRO_URO(1) | \
                  CP15_AREA_ACCESS_PERMISSIONS_PRW_URW(2) | \
                  CP15_AREA_ACCESS_PERMISSIONS_PRW_URW(3) | \
                  CP15_AREA_ACCESS_PERMISSIONS_PRW_URW(4) | \
                  CP15_AREA_ACCESS_PERMISSIONS_PRW_URW(5) | \
                  CP15_AREA_ACCESS_PERMISSIONS_PRW_URW(6) | \
                  CP15_AREA_ACCESS_PERMISSIONS_PRW_URW(7))
    mcr     CP15_REG5_INSTRUCTION_ACCESS_PERMISSION(r0)

    // Data access permission (RW except for region 1, which is RO)
    mcr     CP15_REG5_DATA_ACCESS_PERMISSION(r0)

    // Enable instruction and data caches, ITCM and DTCM
    mrc     CP15_REG1_CONTROL_REGISTER(r0)
    ldr     r1, =(CP15_CONTROL_ITCM_ENABLE | \
                  CP15_CONTROL_DTCM_ENABLE | \
                  CP15_CONTROL_ICACHE_ENABLE | \
                  CP15_CONTROL_DCACHE_ENABLE | \
                  CP15_CONTROL_PROTECTION_UNIT_ENABLE)
    orr     r0, r0, r1
    mcr     CP15_REG1_CONTROL_REGISTER(r0)

    ldr     r0, =masks
    str     r9, [r0]

    bx      lr

// Returns a cached mirror of an address.
BEGIN_ASM_FUNC memCached

    ldr     r1, =masks
    ldr     r1, [r1]
    ldr     r2, [r1], #4
    and     r0, r0, r2
    ldr     r2, [r1]
    orr     r0, r0, r2
    bx      lr

// Returns an uncached mirror of an address.
BEGIN_ASM_FUNC memUncached

    ldr     r1, =masks
    ldr     r1, [r1]
    ldr     r2, [r1], #8
    and     r0, r0, r2
    ldr     r2, [r1]
    orr     r0, r0, r2
    bx      lr

    .data
    .align    2

dsmasks:
    .word   0x003fffff, 0x02000000, 0x02c00000
debugmasks:
    .word   0x007fffff, 0x02000000, 0x02800000
dsimasks:
    .word   0x00ffffff, 0x02000000, 0x0c000000

masks:
    .word   dsmasks
