// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2005 Dave Murphy (WinterMute)
// Copyright (C) 2025 Antonio Niño Díaz

#ifdef ARM7
#include <nds/arm7/i2c.h>
#include <nds/arm7/tmio.h>
#endif
#include <nds/interrupts.h>
#include <nds/ipc.h>
#include <nds/system.h>

void IntrMain(void); // Prototype for assembly interrupt dispatcher

#ifdef ARM9
#define INT_TABLE_SECTION __attribute__((section(".itcm.data")))
#else
#define INT_TABLE_SECTION
#endif

VoidFn irqTable[MAX_INTERRUPTS] INT_TABLE_SECTION;
#ifdef ARM7
VoidFn irqTableAUX[MAX_INTERRUPTS_AUX] TWL_BSS;

static TWL_BSS VoidFn __powerbuttonCB = (VoidFn)0;

TWL_CODE void i2cIRQHandler(void)
{
    // I2CREGPM_PWRIF
    //
    //   Bit 0 = Just released
    //   Bit 1 = Long press
    //   Bit 3 = Just pressed
    //
    // A short press will make bit 3 to be set, and bit 0 is set when the button
    // is released.
    //
    // A long press causes bit 3 to be set, then bit 1. Bit 0 is never set.
    //
    // A very long press causes bit 3 to be set, then bit 1, then the hardware
    // is forcefully shut down.

    bool power_released = i2cReadRegister(I2C_PM, I2CREGPM_PWRIF) & BIT(0);

    if (power_released)
    {
        if (__powerbuttonCB)
        {
            __powerbuttonCB();
        }
        else
        {
            // If the developer hasn't setup a callback, try to reboot and shut
            // down the console.
            systemReboot();
            systemShutDown();
        }
    }
}

TWL_CODE void irqInitAUX(void)
{
    // Set all interrupts to dummy functions.
    for (int i = 0; i < MAX_INTERRUPTS_AUX; i++)
        irqTableAUX[i] = NULL;
}

VoidFn setPowerButtonCB(VoidFn CB)
{
    if (!isDSiMode())
        return CB;

    VoidFn tmp = __powerbuttonCB;
    __powerbuttonCB = CB;
    return tmp;
}
#endif

static void __irqSet(u32 mask, VoidFn handler, VoidFn irqTable_[], u32 max)
{
    // Set every bit in order. Skip bits which are out of bounds.
    while (mask)
    {
        u32 i = __builtin_clz(mask) ^ 0x1F;
        if (i < max)
            irqTable_[i] = handler;
        mask &= ~(1 << i);
    }
}

void irqSet(u32 mask, VoidFn handler)
{
    int oldIME = enterCriticalSection();

    __irqSet(mask, handler, irqTable, MAX_INTERRUPTS);

    if (mask & IRQ_VBLANK)
        REG_DISPSTAT |= DISP_VBLANK_IRQ;
    if (mask & IRQ_HBLANK)
        REG_DISPSTAT |= DISP_HBLANK_IRQ;
    if (mask & IRQ_IPC_SYNC)
        REG_IPC_SYNC |= IPC_SYNC_IRQ_ENABLE;

    leaveCriticalSection(oldIME);
}

void irqInitHandler(VoidFn handler)
{
    REG_IME = 0;
    REG_IE = 0;
    REG_IF = ~0;

#ifdef ARM7
    if (isDSiMode())
    {
        REG_AUXIE = 0;
        REG_AUXIF = ~0;
    }
#endif
    IRQ_HANDLER = handler;
}

void irqInit(void)
{
    irqInitHandler(IntrMain);

    // Set all interrupts to dummy functions.
    for (int i = 0; i < MAX_INTERRUPTS; i++)
        irqTable[i] = NULL;

#ifdef ARM7
    if (isDSiMode())
    {
        irqInitAUX();
        irqSetAUX(IRQ_I2C, i2cIRQHandler);
        irqEnableAUX(IRQ_I2C);
        TMIO_init();
    }
#endif

    REG_IME = 1; // Enable interrupts
}

void irqEnable(uint32_t irq)
{
    int oldIME = enterCriticalSection();

    if (irq & IRQ_VBLANK)
        REG_DISPSTAT |= DISP_VBLANK_IRQ;
    if (irq & IRQ_HBLANK)
        REG_DISPSTAT |= DISP_HBLANK_IRQ;
    if (irq & IRQ_VCOUNT)
        REG_DISPSTAT |= DISP_YTRIGGER_IRQ;
    if (irq & IRQ_IPC_SYNC)
        REG_IPC_SYNC |= IPC_SYNC_IRQ_ENABLE;

    REG_IE |= irq;

    leaveCriticalSection(oldIME);
}

void irqDisable(uint32_t irq)
{
    int oldIME = enterCriticalSection();

    if (irq & IRQ_VBLANK)
        REG_DISPSTAT &= ~DISP_VBLANK_IRQ;
    if (irq & IRQ_HBLANK)
        REG_DISPSTAT &= ~DISP_HBLANK_IRQ;
    if (irq & IRQ_VCOUNT)
        REG_DISPSTAT &= ~DISP_YTRIGGER_IRQ;
    if (irq & IRQ_IPC_SYNC)
        REG_IPC_SYNC &= ~IPC_SYNC_IRQ_ENABLE;

    REG_IE &= ~irq;

    leaveCriticalSection(oldIME);
}

void irqClear(u32 mask)
{
    int oldIME = enterCriticalSection();

    __irqSet(mask, NULL, irqTable, MAX_INTERRUPTS);
    irqDisable(mask);

    leaveCriticalSection(oldIME);
}

#ifdef ARM7

TWL_CODE void irqSetAUX(u32 mask, VoidFn handler)
{
    int oldIME = enterCriticalSection();

    __irqSet(mask, handler, irqTableAUX, MAX_INTERRUPTS_AUX);

    leaveCriticalSection(oldIME);
}

TWL_CODE void irqClearAUX(u32 mask)
{
    int oldIME = enterCriticalSection();

    __irqSet(mask, NULL, irqTableAUX, MAX_INTERRUPTS_AUX);
    irqDisableAUX(mask);

    leaveCriticalSection(oldIME);
}

TWL_CODE void irqDisableAUX(uint32_t irq)
{
    int oldIME = enterCriticalSection();

    REG_AUXIE &= ~irq;

    leaveCriticalSection(oldIME);
}

TWL_CODE void irqEnableAUX(uint32_t irq)
{
    int oldIME = enterCriticalSection();

    REG_AUXIE |= irq;

    leaveCriticalSection(oldIME);
}

#endif
