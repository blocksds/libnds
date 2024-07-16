// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2009-2017 Dave Murphy (WinterMute)
// Copyright (C) 2017 fincs
// Copyright (c) 2023 Antonio Niño Díaz

// List of MPU regions
// ===================
//
// The base addresses of ITCM and DTCM are defined by the linkerscript.
//
// The base address of ITCM is fixed at 0x00000000, but it is mirrored for 32 MB
// every 32 KB. This means that 0x01000000 is a valid base address.
//
// Num |    Base    |  Size  | System    | Access | Cache | WB | Description
// ====+============+========+===========+========+=======+====+=======================
//   0 | 0x04000000 |  64 MB | All       |  R/W   |   N   | N  | I/O registers
// ----+------------+--------+-----------+--------+-------+----+-----------------------
//   1 | 0xFFFF0000 |  64 KB | All       |  RO    |   Y   | N  | System ROM
// ----+------------+--------+-----------+--------+-------+----+-----------------------
//   2 | 0x00000000 |   4 KB | All       |  R/W   |   N   | N  | Alternate vector base
// ----+------------+--------+-----------+--------+-------+----+-----------------------
//   3 | 0x08000000 | 128 MB | DS, DSd   |  R/W   |   N   | N  | DS Accessory (GBA Cart)
//     | 0x03000000 |   8 MB | DSI, DSId |        |       |    | DSi switchable IWRAM
// ----+------------+--------+-----------+--------+-------+----+-----------------------
//   4 | 0x01000000 |  32 KB | All       |  R/W   |   N   | N  | ITCM
// ----+------------+--------+-----------+--------+-------+----+-----------------------
//   5 | 0x02000000 |  16 MB | DS    [1] |  R/W   |   N   | N  | Non-cacheable main RAM
//     | 0x02800000 |   8 MB | DSd       |        |       |    |
//     | 0x0C000000 |  16 MB | DSI       |        |       |    |
//     | 0x0C000000 |  32 MB | DSId      |        |       |    | DSi debugger extended IWRAM
// ----+------------+--------+-----------+--------+-------+----+-----------------------
//   6 | 0x02000000 |   4 MB | DS        |  R/W   |   Y   | Y  | Cacheable main RAM
//     | 0x02000000 |   8 MB | DSd   [2] |        |       |    |
//     | 0x02000000 |  16 MB | DSI, DSId |        |       |    |
// ----+------------+--------+-----------+--------+-------+----+-----------------------
//   7 | 0x02FF0000 |  16 KB | All       |  R/W   |   N   | N  | DTCM
//
// [1]: The size of the main RAM of the DS is 4 MB. This is mirrored up to
// 0x03000000 (4 times in total). The last mirror is often used for
// optimizations that involve accessing the end of main RAM (like the BIOS
// interrupt flags). The other two mirrors aren't used by libnds, but the mirror
// at 0x02400000 is used by some legacy applications built with libnds.
//
// Also, note that this section overlaps with the cacheable main RAM (region 7).
// This is required because the size of the regions must be a power of two (and
// 12 MB isn't a power of two), and regions with a higher index have priority
// over regions with a lower index (so the cacheable region has priority).
//
// It also overlaps with the DTCM region, which has a higher priority than both
// the cacheable and non-cacheable regions. This region is required to disable
// the data cache in DTCM.
//
// [2]: The actual size of the main RAM of the DSi debugger version is 32 MB,
// but it isn't possible to map everything at 0x02000000 because shared WRAM is
// mapped at 0x03000000, so there are only 16 MB available. The last 16 MB of
// the main RAM can only be accessed from their non-cachable mirror. This isn't
// a problem in a regular retail DSi because it has exactly 16 MB of RAM.
//
// Legend:
//
//   Access: Data and instruction access permissions (same for privileged and user)
//   Cache: Data and instruction cacheable
//   WB: Write buffer enable
//
//   DS: Regular DS
//   DSd: Debugger DS
//   DSI: Regular DSi
//   DSId: Debugger DSi

#include <nds/arm9/cp15_asm.h>
#include <nds/asminc.h>

#define REGION_IO_REGISTERS         0
#define REGION_SYSTEM_ROM           1
#define REGION_ALT_VECTORS          2
#define REGION_SLOT_2_DSI_IWRAM     3 // DS: Slot-2 | DSi: Switchable IWRAM.
#define REGION_ITCM                 4
#define REGION_RAM_UNCACHED         5
#define REGION_RAM_CACHED           6
#define REGION_DTCM                 7

    .syntax  unified
    .arch    armv5te
    .cpu     arm946e-s

    .arm

// This macro sets the zero flag to 1 when running on a DSi; otherwise 0.
.macro IS_DSI reg
    // If the lowest two bits of SCFG_A9ROM are 0b01, this is a DSi.

    ldr     \reg, =0x04004000 // SCFG_A9ROM
    ldrb    \reg, [\reg]
    and     \reg, \reg, #0x3
    cmp     \reg, #0x1
.endm

// This macro sets the zero flag to 0 if the DSi is a regular retail unit; it
// will set it to 1 if it's a DSi debugger unit.
.macro IS_DSI_DEBUGGER reg1, reg2
    // In order to detect if there are 16 MB of RAM or open bus, we write two
    // different values to 0x0DFFFFFA and try to read them back. If they match,
    // the memory is present. If not, it's open bus.

    // Set RAM size to 32 MB. This will turn the memory from 0x0D000000 to
    // 0x0DFFFFFF into open bus in the case of a retail DSi unit, and into
    // additional 16 MB of RAM in a debugger DSi unit.
    ldr     \reg1, =0x4004008 // SCFG_EXT9
    ldr     \reg2, [\reg1]
    orr     \reg2, #0xC000 // 32 MB
    str     \reg2, [\reg1]

    // Try to write and read from the potential RAM region.

    ldr     \reg1, =0x0DFFFFFA

    mov     \reg2, #0x55
    strb    \reg2, [\reg1]
    ldrb    \reg2, [\reg1]
    cmp     \reg2, #0x55

    // If the zero flag is set at this point, the value we read matched the
    // value we wrote, so there is potentially RAM in that address. Try with a
    // different value in case it was just a coincidence.
    //
    // If the zero flag isn't set, the values don't match, no need to try again
    // because there isn't any RAM there.

    moveq   \reg2, #0xAA
    strbeq  \reg2, [\reg1]
    ldrbeq  \reg2, [\reg1]
    cmpeq   \reg2, #0xAA

    // If the zero flag is set the memory is definitely present (= debugger DSi)

    // If the zero flag is not set this is a retail DSi, reduce the amount of
    // RAM to 16 MB.

    ldrne   \reg1, =0x4004008 // SCFG_EXT9
    ldrne   \reg2, [\reg1]
    bicne   \reg2, #0x4000
    strne   \reg2, [\reg1]
.endm

// Returns:
// r7: 1 if the device is a debugger unit, 0 if it is a retail unit.
// r8: End address of the cached RAM for this DS model (mapped at 0x02000000)
BEGIN_ASM_FUNC __libnds_mpu_setup

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

    // DTCM base is moveable. Set it to __dtcm_start
    // DTCM size = 16 KB, not mirrored
    ldr     r0, =__dtcm_start
    orr     r0, r0, #(CP15_TCM_SIZE_16KB << 1)
    mcr     CP15_REG9_DTCM_CONTROL(r0)

    // ITCM base is not moveable. Fixed to 0x00000000.
    // ITCM size = 32 KB, mirrored every 32KB up to 32MB
    mov     r0, #(0x00000000 | (CP15_TCM_SIZE_32MB << 1))
    mcr     CP15_REG9_ITCM_CONTROL(r0)

    // Setup memory regions similar to Release Version

    // IO registers
    ldr     r0, =(0x04000000 | CP15_REGION_SIZE_64MB | CP15_CONFIG_REGION_ENABLE)
    mcr     CP15_REG6_PROTECTION_REGION(r0, REGION_IO_REGISTERS)

    // System ROM
    ldr     r0, =(0xFFFF0000 | CP15_REGION_SIZE_64KB | CP15_CONFIG_REGION_ENABLE)
    mcr     CP15_REG6_PROTECTION_REGION(r0, REGION_SYSTEM_ROM)

    // Alternate vector base
    ldr     r0, =(0x00000000 | CP15_REGION_SIZE_4KB | CP15_CONFIG_REGION_ENABLE)
    mcr     CP15_REG6_PROTECTION_REGION(r0, REGION_ALT_VECTORS)

    // DTCM
    ldr     r0, =__dtcm_start
    orr     r0, r0, #(CP15_REGION_SIZE_16KB | CP15_CONFIG_REGION_ENABLE)
    mcr     CP15_REG6_PROTECTION_REGION(r0, REGION_DTCM)

    // ITCM
    ldr     r0, =__itcm_start
    mov     r0, r0, lsr #15 // Align to 32k
    mov     r0, r0, lsl #15
    orr     r0, r0, #(CP15_REGION_SIZE_32KB | CP15_CONFIG_REGION_ENABLE)
    mcr     CP15_REG6_PROTECTION_REGION(r0, REGION_ITCM)

    // When running on a DSi, load DSi code/data sections
    IS_DSI  r0
    beq     dsi_mode

    // DS mode: The debugger model is detected using swiIsDebugger()

    swi     0xF0000 // swiIsDebugger. Only in DS mode, cache must be off.

    ldr     r1, =(0x08000000 | CP15_REGION_SIZE_128MB | CP15_CONFIG_REGION_ENABLE)
    cmp     r0, #0
    bne     debug_mode

    mov     r7, #0 // Retail DS
    ldr     r3, =(0x02000000 | CP15_REGION_SIZE_4MB | CP15_CONFIG_REGION_ENABLE)
    ldr     r2, =(0x02000000 | CP15_REGION_SIZE_16MB | CP15_CONFIG_REGION_ENABLE)
    mov     r8, #0x02400000

    ldr     r9, =dsmasks
    b       setregions

debug_mode:
    mov     r7, #1 // Debugger DS
    ldr     r3, =(0x02000000 | CP15_REGION_SIZE_8MB | CP15_CONFIG_REGION_ENABLE)
    ldr     r2, =(0x02800000 | CP15_REGION_SIZE_8MB | CP15_CONFIG_REGION_ENABLE)
    mov     r8, #0x02800000
    ldr     r9, =debugmasks
    b       setregions

    // DSi mode: The debugger model is detected by checking the RAM size.

dsi_mode:
    // This will set the zero flag to 1 if this is a debugger DSi
    IS_DSI_DEBUGGER r0, r1

    ldr     r1, =(0x03000000 | CP15_REGION_SIZE_8MB | CP15_CONFIG_REGION_ENABLE)
    ldr     r3, =(0x02000000 | CP15_REGION_SIZE_16MB | CP15_CONFIG_REGION_ENABLE)
    // Retail DSi
    movne   r7, #0
    ldrne   r2, =(0x0C000000 | CP15_REGION_SIZE_16MB | CP15_CONFIG_REGION_ENABLE)
    // Debugger DSi
    moveq   r7, #1
    ldreq   r2, =(0x0C000000 | CP15_REGION_SIZE_32MB | CP15_CONFIG_REGION_ENABLE)
    mov     r8, #0x03000000 // The end is after 16 MB even on a debugger
    ldr     r9, =dsimasks

setregions:

    // DS Accessory (GBA Cart) / DSi switchable IWRAM
    mcr     CP15_REG6_PROTECTION_REGION(r1, REGION_SLOT_2_DSI_IWRAM)

    // Non-cacheable main RAM
    mcr     CP15_REG6_PROTECTION_REGION(r2, REGION_RAM_UNCACHED)

    // Cacheable main RAM
    mcr     CP15_REG6_PROTECTION_REGION(r3, REGION_RAM_CACHED)

    // Write buffer enable for cacheable main RAM
    ldr     r0, =CP15_CONFIG_AREA_IS_BUFFERABLE(REGION_RAM_CACHED)
    mcr     CP15_REG3_WRITE_BUFFER_CONTROL(r0)

    // Enable data and instruction caches for the system ROM and cacheable main
    // RAM regions.
    ldr     r0, =(CP15_CONFIG_AREA_IS_CACHABLE(REGION_SYSTEM_ROM) | \
                  CP15_CONFIG_AREA_IS_CACHABLE(REGION_RAM_CACHED))
    mcr     CP15_REG2_DATA_CACHE_CONFIG(r0)
    mcr     CP15_REG2_INSTRUCTION_CACHE_CONFIG(r0)

    // Instruction access permission. All regions are RW except for:
    // - System ROM. It's read-only.
    // - DTCM. The CPU can´t execute code from here.
    ldr     r0, =(CP15_AREA_ACCESS_PERMISSIONS_PRW_URW(REGION_IO_REGISTERS) | \
                  CP15_AREA_ACCESS_PERMISSIONS_PRO_URO(REGION_SYSTEM_ROM) | \
                  CP15_AREA_ACCESS_PERMISSIONS_PRW_URW(REGION_ALT_VECTORS) | \
                  CP15_AREA_ACCESS_PERMISSIONS_PRW_URW(REGION_SLOT_2_DSI_IWRAM) | \
                  CP15_AREA_ACCESS_PERMISSIONS_PRW_URW(REGION_ITCM) | \
                  CP15_AREA_ACCESS_PERMISSIONS_PRW_URW(REGION_RAM_UNCACHED) | \
                  CP15_AREA_ACCESS_PERMISSIONS_PRW_URW(REGION_RAM_CACHED) | \
                  CP15_AREA_ACCESS_PERMISSIONS_PNO_UNO(REGION_DTCM))
    mcr     CP15_REG5_INSTRUCTION_ACCESS_PERMISSION(r0)

    // Data access permission. All regions are RW except for:
    // - System ROM. It's read-only.
    ldr     r0, =(CP15_AREA_ACCESS_PERMISSIONS_PRW_URW(REGION_IO_REGISTERS) | \
                  CP15_AREA_ACCESS_PERMISSIONS_PRO_URO(REGION_SYSTEM_ROM) | \
                  CP15_AREA_ACCESS_PERMISSIONS_PRW_URW(REGION_ALT_VECTORS) | \
                  CP15_AREA_ACCESS_PERMISSIONS_PRW_URW(REGION_SLOT_2_DSI_IWRAM) | \
                  CP15_AREA_ACCESS_PERMISSIONS_PRW_URW(REGION_ITCM) | \
                  CP15_AREA_ACCESS_PERMISSIONS_PRW_URW(REGION_RAM_UNCACHED) | \
                  CP15_AREA_ACCESS_PERMISSIONS_PRW_URW(REGION_RAM_CACHED) | \
                  CP15_AREA_ACCESS_PERMISSIONS_PRW_URW(REGION_DTCM))
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

// Enables data cache for the DS slot-2 memory region
BEGIN_ASM_FUNC peripheralSlot2EnableCache

    // When running on a DSi there is no slot-2 memory region, just return
    IS_DSI  r1
    bxeq    lr

    // Enable data cache for this region
    mrc     CP15_REG2_DATA_CACHE_CONFIG(r1)
    orr     r1, CP15_CONFIG_AREA_IS_CACHABLE(REGION_SLOT_2_DSI_IWRAM)
    mcr     CP15_REG2_DATA_CACHE_CONFIG(r1)

    // If write-through is requested there is nothing else to do
    tst     r0, r0
    bxeq    lr

    // Write buffer enable if requested (write-back instead of write-through)
    mrc     CP15_REG3_WRITE_BUFFER_CONTROL(r1)
    orr     r1, CP15_CONFIG_AREA_IS_BUFFERABLE(REGION_SLOT_2_DSI_IWRAM)
    mcr     CP15_REG3_WRITE_BUFFER_CONTROL(r1)

    bx      lr

// Disable data cache for the DS slot-2 memory region
BEGIN_ASM_FUNC peripheralSlot2DisableCache

    // When running on a DSi there is no slot-2 memory region, just return
    IS_DSI  r1
    bxeq    lr

    // If write-back is enabled, flush all data cache before disabling it for
    // this region
    mrc     CP15_REG3_WRITE_BUFFER_CONTROL(r0)
    tst     r0, CP15_CONFIG_AREA_IS_BUFFERABLE(REGION_SLOT_2_DSI_IWRAM)
    beq     .write_through

    // Disable write buffer
    bic     r0, CP15_CONFIG_AREA_IS_BUFFERABLE(REGION_SLOT_2_DSI_IWRAM)
    mcr     CP15_REG3_WRITE_BUFFER_CONTROL(r0)

    push    {lr}
    bl      CP15_CleanAndFlushDCache
    pop     {lr}
.write_through:

    // Disable data cache
    mrc     CP15_REG2_DATA_CACHE_CONFIG(r0)
    bic     r0, CP15_CONFIG_AREA_IS_CACHABLE(REGION_SLOT_2_DSI_IWRAM)
    mcr     CP15_REG2_DATA_CACHE_CONFIG(r0)
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
