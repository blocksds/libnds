// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2017 Dave Murphy (WinterMute)

#include <nds/bios.h>
#include <nds/interrupts.h>
#include <nds/system.h>

#define BASE_DELAY (100)

void twlEnableSlot1(void)
{
    int oldIME = enterCriticalSection();

    while ((REG_SCFG_MC & SCFG_MC_PWR_MASK) == SCFG_MC_PWR_REQUEST_OFF)
        swiDelay(1 * BASE_DELAY);

    if ((REG_SCFG_MC & SCFG_MC_PWR_MASK) == SCFG_MC_PWR_OFF)
    {
        REG_SCFG_MC = (REG_SCFG_MC & ~SCFG_MC_PWR_MASK) | SCFG_MC_PWR_RESET;
        swiDelay(10 * BASE_DELAY);
        REG_SCFG_MC = (REG_SCFG_MC & ~SCFG_MC_PWR_MASK) | SCFG_MC_PWR_ON;
        swiDelay(10 * BASE_DELAY);
    }

    leaveCriticalSection(oldIME);
}

void twlDisableSlot1(void)
{
    int oldIME = enterCriticalSection();

    while ((REG_SCFG_MC & SCFG_MC_PWR_MASK) == SCFG_MC_PWR_REQUEST_OFF)
        swiDelay(1 * BASE_DELAY);

    if ((REG_SCFG_MC & SCFG_MC_PWR_MASK) == SCFG_MC_PWR_ON)
    {
        REG_SCFG_MC = (REG_SCFG_MC & ~SCFG_MC_PWR_MASK) | SCFG_MC_PWR_REQUEST_OFF;
        while ((REG_SCFG_MC & SCFG_MC_PWR_MASK) != SCFG_MC_PWR_OFF)
            swiDelay(1 * BASE_DELAY);
    }

    leaveCriticalSection(oldIME);
}
