// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2005 Dave Murphy (WinterMute)

#ifdef ARM7
#include <nds/arm7/i2c.h>
#include <nds/arm7/tmio.h>
#endif
#include <nds/interrupts.h>
#include <nds/ipc.h>
#include <nds/system.h>

void IntrMain(void); // Prototype for assembly interrupt dispatcher

void irqDummy(void)
{
}

#ifdef ARM9
#define INT_TABLE_SECTION __attribute__((section(".itcm")))
#else
#define INT_TABLE_SECTION
#endif

struct IntTable irqTable[MAX_INTERRUPTS] INT_TABLE_SECTION;
#ifdef ARM7
struct IntTable irqTableAUX[MAX_INTERRUPTS_AUX] TWL_BSS;

static TWL_BSS VoidFn __powerbuttonCB = (VoidFn)0;

TWL_CODE void i2cIRQHandler(void)
{
    int cause = (i2cReadRegister(I2C_PM, I2CREGPM_PWRIF) & 0x3)
              | (i2cReadRegister(I2C_GPIO, 0x02) << 2);

    switch (cause & 3)
    {
        case 1:
            if (__powerbuttonCB)
            {
                __powerbuttonCB();
            }
            else
            {
                i2cWriteRegister(I2C_PM, I2CREGPM_RESETFLAG, 1);
                i2cWriteRegister(I2C_PM, I2CREGPM_PWRCNT, 1);
            }
            break;
        case 2:
            writePowerManagement(PM_CONTROL_REG,PM_SYSTEM_PWR);
            break;
    }
}

TWL_CODE void irqInitAUX(void)
{
    // Set all interrupts to dummy functions.
    for (int i = 0; i < MAX_INTERRUPTS_AUX; i++)
    {
        irqTableAUX[i].handler = irqDummy;
        irqTableAUX[i].mask = 0;
    }
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

static void __irqSet(u32 mask, IntFn handler, struct IntTable irqTable[], u32 max)
{
    if (mask == 0)
        return;

    u32 i;

    for (i = 0; i < max; i++)
    {
        if (!irqTable[i].mask || irqTable[i].mask == mask)
            break;
    }

    if (i == max)
        return;

    irqTable[i].handler = handler;
    irqTable[i].mask = mask;
}

void irqSet(u32 mask, IntFn handler)
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

void irqInitHandler(IntFn handler)
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
    {
        irqTable[i].handler = irqDummy;
        irqTable[i].mask = 0;
    }

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
    if(irq & IRQ_IPC_SYNC)
        REG_IPC_SYNC &= ~IPC_SYNC_IRQ_ENABLE;

    REG_IE &= ~irq;

    leaveCriticalSection(oldIME);
}

static void __irqClear(u32 mask, struct IntTable irqTable[], u32 max)
{
    u32 i = 0;

    for (i = 0; i < max; i++)
    {
        if (irqTable[i].mask == mask)
            break;
    }

    if (i == max)
        return;

    irqTable[i].handler = irqDummy;
}

void irqClear(u32 mask)
{
    int oldIME = enterCriticalSection();

    __irqClear(mask, irqTable, MAX_INTERRUPTS);
    irqDisable(mask);

    leaveCriticalSection(oldIME);
}

#ifdef ARM7

TWL_CODE void irqSetAUX(u32 mask, IntFn handler)
{
    int oldIME = enterCriticalSection();

    __irqSet(mask, handler, irqTableAUX, MAX_INTERRUPTS_AUX);

    leaveCriticalSection(oldIME);
}

TWL_CODE void irqClearAUX(u32 mask)
{
    int oldIME = enterCriticalSection();

    __irqClear(mask, irqTableAUX, MAX_INTERRUPTS_AUX);
    irqDisable(mask);

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
