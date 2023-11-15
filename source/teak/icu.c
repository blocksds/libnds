// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2020 Gericom

#include <teak/teak.h>

void icuInit(void)
{
    cpuDisableIrqs();

    // Ensure that all interrupts are disabled
    REG_ICU_IRQ_DISABLE = 0xFFFF;

    // Acknowledge all interrupts in case any of them was active
    REG_ICU_IRQ_ACK = 0xFFFF;

    REG_ICU_IRQ_INT0 = 0;
    REG_ICU_IRQ_INT1 = 0;
    REG_ICU_IRQ_INT2 = 0;
    REG_ICU_IRQ_VINT = 0;
    REG_ICU_IRQ_MODE = 0;

    // Use normal polarity
    REG_ICU_IRQ_POLARITY = 0;

    cpuDisableInt0();
    cpuDisableInt1();
    cpuDisableInt2();
    cpuDisableVInt();
    cpuEnableIrqs();
}

void icuIrqSetup(u16 mask, int index)
{
    // Setup interrupt as triggered by edge
    REG_ICU_IRQ_MODE |= mask;

    // Set normal polarity
    REG_ICU_IRQ_POLARITY = ~mask;

    // Remove the mask that disables this interrupt
    REG_ICU_IRQ_DISABLE = ~mask;

    // Clear pending IRQ just in case
    REG_ICU_IRQ_ACK = mask;

    // Route hardware ingerrupt to the right CPU interrupt
    if (index == 0)
        REG_ICU_IRQ_INT0 = mask;
    else if (index == 1)
        REG_ICU_IRQ_INT1 = mask;
    else if (index == 2)
        REG_ICU_IRQ_INT2 = mask;
}

void icuIrqDisable(u16 mask)
{
    // Set mask that disables this interrupt
    REG_ICU_IRQ_DISABLE |= mask;

    // Clear pending IRQ just in case
    REG_ICU_IRQ_ACK = mask;
}
