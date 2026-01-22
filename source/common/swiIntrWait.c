// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2025 Antonio Niño Díaz

// This implements some BIOS functions that are buggy in the real BIOS. The bugs
// depend on the CPU and the NDS model, so it's better to reimplement them
// ourselves. The compiler does a pretty good job, so it's better to have them
// as C than as hand-written ASM.

#include <nds/bios.h>
#include <nds/interrupts.h>

#pragma GCC optimize ("O3")

ARM_CODE ITCM_CODE
void swiWaitForVBlank(void)
{
    swiIntrWait(INTRWAIT_CLEAR_FLAGS, IRQ_VBLANK);
}

LIBNDS_NOINLINE // So that it isn't inlined in swiWaitForVBlank()
ARM_CODE ITCM_CODE
void swiIntrWait(u32 clearOldFlags, uint32_t flags)
{
    REG_IME = 0;

    vuint32 *bios_flags = &__irq_flags[0];

    if (clearOldFlags == INTRWAIT_CLEAR_FLAGS)
        *bios_flags &= ~flags;

    while (1)
    {
        uint32_t old_flags = *bios_flags;
        uint32_t irqs_found = old_flags & flags;
        *bios_flags = old_flags & ~irqs_found;

        if (irqs_found)
            break;

        REG_IME = 1;

#ifdef ARM9
        // Any register works for this as long as its value is zero
        asm volatile inline (
            "mcr p15, 0, %0, c7, c0, 4\n\t" // CP15_REG7_WAIT_FOR_INTERRUPT
            :           // Outputs
            : "r"(0)    // Inputs
            :           // Clobber list
        );
#else
        swiHalt();
#endif

        REG_IME = 0;
    }

    REG_IME = 1;
}

#ifdef ARM7
ARM_CODE
void swiIntrWaitAUX(u32 clearOldFlags, uint32_t flags, uint32_t aux_flags)
{
    REG_IME = 0;

    vuint32 *bios_flags = &__irq_flags[0];
    vuint32 *bios_flags_aux = &__irq_flagsaux[0];

    if (clearOldFlags == INTRWAIT_CLEAR_FLAGS)
    {
        *bios_flags &= ~flags;
        *bios_flags_aux &= ~aux_flags;
    }

    while (1)
    {
        uint32_t old_flags = *bios_flags;
        uint32_t irqs_found = old_flags & flags;
        *bios_flags = old_flags & ~irqs_found;

        uint32_t old_flags_aux = *bios_flags_aux;
        uint32_t irqs_found_aux = old_flags_aux & aux_flags;
        *bios_flags_aux = old_flags_aux & ~irqs_found_aux;

        if (irqs_found | irqs_found_aux)
            break;

        REG_IME = 1;
        swiHalt();
        REG_IME = 0;
    }

    REG_IME = 1;
}
#endif // ARM7
