// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2005 Michael Noland (joat)
// Copyright (C) 2005 Jason Rogers (dovoto)
// Copyright (C) 2005-2011 Dave Murphy (WinterMute)
// Copyright (C) 2007 Mike Parks (BigRedPimp)

#include <nds/arm9/video.h>
#include <nds/dma.h>
#include <nds/ndstypes.h>

u32 vramSetPrimaryBanks(VRAM_A_TYPE a, VRAM_B_TYPE b, VRAM_C_TYPE c, VRAM_D_TYPE d)
{
    uint32 vramTemp = VRAM_CR;

    VRAM_A_CR = VRAM_ENABLE | a;
    VRAM_B_CR = VRAM_ENABLE | b;
    VRAM_C_CR = VRAM_ENABLE | c;
    VRAM_D_CR = VRAM_ENABLE | d;

    return vramTemp;
}

u32 vramSetMainBanks(VRAM_A_TYPE a, VRAM_B_TYPE b, VRAM_C_TYPE c, VRAM_D_TYPE d)
{
    return vramSetPrimaryBanks(a, b, c, d);
}

u32 vramSetBanks_EFG(VRAM_E_TYPE e, VRAM_F_TYPE f, VRAM_G_TYPE g)
{
    uint32 vramTemp = VRAM_EFG_CR;

    VRAM_E_CR = VRAM_ENABLE | e;
    VRAM_F_CR = VRAM_ENABLE | f;
    VRAM_G_CR = VRAM_ENABLE | g;

    return vramTemp;
}

void vramRestorePrimaryBanks(u32 vramTemp)
{
    VRAM_CR = vramTemp;
}

void vramRestoreMainBanks(u32 vramTemp)
{
    VRAM_CR = vramTemp;
}

void vramRestoreBanks_EFG(u32 vramTemp)
{
    VRAM_EFG_CR = vramTemp;
}

void setBrightness(int screen, int level)
{
    int mode = 1 << 14;

    if (level < 0)
    {
        level = -level;
        mode = 2 << 14;
    }

    if (level > 16)
        level = 16;

    if (screen & 1)
        REG_MASTER_BRIGHT = mode | level;
    if (screen & 2)
        REG_MASTER_BRIGHT_SUB = mode | level;
}

u32 __attribute__((weak)) vramDefault(void)
{
    // Map all VRAM banks to LCDC mode
    VRAM_CR = 0x80808080;
    VRAM_E_CR = 0x80;
    VRAM_F_CR = 0x80;
    VRAM_G_CR = 0x80;
    VRAM_H_CR = 0x80;
    VRAM_I_CR = 0x80;

    dmaFillWords(0, BG_PALETTE, 2 * 1024); // Clear main and sub palette
    dmaFillWords(0, OAM, 2 * 1024);        // Clear main and sub OAM
    dmaFillWords(0, VRAM, 656 * 1024);     // Clear all VRAM

    return vramSetPrimaryBanks(VRAM_A_MAIN_BG, VRAM_B_MAIN_SPRITE, VRAM_C_SUB_BG,
                               VRAM_D_SUB_SPRITE);
}
