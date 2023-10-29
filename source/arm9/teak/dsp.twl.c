// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2020 Gericom
// Copyright (C) 2023 Antonio Niño Díaz

#include <nds.h>

#include <nds/arm9/teak/dsp.h>

void dspSetBlockReset(bool reset)
{
    REG_SCFG_RST = reset ? SCFG_RST_DSP_APPLY : SCFG_RST_DSP_RELEASE;
}

void dspSetClockEnabled(bool enabled)
{
    if (enabled)
        REG_SCFG_CLK |= SCFG_CLK_DSP;
    else
        REG_SCFG_CLK &= ~SCFG_CLK_DSP;
}

void dspResetInterface()
{
    dspSpinWait();

    if ((REG_DSP_PCFG & DSP_PCFG_RESET) == 0)
        return;

    REG_DSP_PCFG &= ~(DSP_PCFG_IE_REP0 | DSP_PCFG_IE_REP1 | DSP_PCFG_IE_REP2);
    REG_DSP_PSEM = 0;
    REG_DSP_PCLEAR = 0xFFFF;

    // Clear reply registers
    vu16 tmp = REG_DSP_REP0;
    tmp = REG_DSP_REP1;
    tmp = REG_DSP_REP2;
    (void)tmp;
}

void dspSetCoreResetOn()
{
    dspSpinWait();

    if (REG_DSP_PCFG & DSP_PCFG_RESET)
        return;

    REG_DSP_PCFG |= DSP_PCFG_RESET;
    dspSpinWait();

    while (REG_DSP_PSTS & DSP_PSTS_PERI_RESET);
}

void dspSetCoreResetOff(u16 repIrqMask)
{
    dspSpinWait();

    while (REG_DSP_PSTS & DSP_PSTS_PERI_RESET);

    dspResetInterface();
    dspSpinWait();
    REG_DSP_PCFG |= (repIrqMask & 7) << DSP_PCFG_IE_REP_SHIFT;
    dspSpinWait();
    REG_DSP_PCFG &= ~DSP_PCFG_RESET;
}

void dspPowerOn()
{
    REG_SCFG_EXT |= SCFG_EXT_DSP | SCFG_EXT_INTERRUPT;

    dspSetBlockReset(true);
    dspSetClockEnabled(true);
    dspSpinWait();
    dspSetBlockReset(false);
    dspResetInterface();
    dspSetCoreResetOn();
}

void dspPowerOff()
{
    REG_SCFG_EXT &= ~(SCFG_EXT_DSP | SCFG_EXT_INTERRUPT);

    dspSetBlockReset(true);
    dspSetClockEnabled(false);
}

void dspSendData(int id, u16 data)
{
    dspSpinWait();
    while (REG_DSP_PSTS & (1 << (DSP_PSTS_CMD_UNREAD_SHIFT + id)));
    (&REG_DSP_CMD0)[4 * id] = data;
}

bool dspSendDataReady(int id)
{
    dspSpinWait();
    return (REG_DSP_PSTS & (1 << (DSP_PSTS_CMD_UNREAD_SHIFT + id))) == 0;
}

u16 dspReceiveData(int id)
{
    dspSpinWait();
    while ((REG_DSP_PSTS & (1 << (DSP_PSTS_REP_NEW_SHIFT + id))) == 0);
    return (&REG_DSP_REP0)[4 * id];
}

bool dspReceiveDataReady(int id)
{
    dspSpinWait();
    return (REG_DSP_PSTS & (1 << (DSP_PSTS_REP_NEW_SHIFT + id))) ? true : false;
}
