// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2005 Christian Auby (DesktopMan)
// Copyright (C) 2005 Dave Murphy (WinterMute)

// Key input code. Provides slightly higher level input forming.

#include <stdlib.h>

#include <nds/arm9/input.h>
#include <nds/ipc.h>
#include <nds/input.h>
#include <nds/system.h>

#include "common/libnds_internal.h"

static uint16_t keys_cur(void)
{
    const uint16_t keyinput_mask = KEY_A | KEY_B | KEY_SELECT | KEY_START |
        KEY_RIGHT | KEY_LEFT | KEY_UP | KEY_DOWN | KEY_R | KEY_L;

    uint16_t keyinput = ~REG_KEYINPUT;
    uint16_t keyxy = ~__transferRegion()->buttons;

    uint16_t keys_arm9 = keyinput & keyinput_mask;

    // Bits 0 and 1 of REG_KEYXY to bits 10 and 11 of KEYPAD_BITS
    uint16_t keys_arm7_xy = (keyxy & (KEYXY_X | KEYXY_Y)) << 10;

    // Bit 3 of REG_KEYXY to bit 14 of KEYPAD_BITS
    uint16_t keys_arm7_debug = (keyxy & KEYXY_DEBUG) << 11;

    // Bits 6 and 7 of REG_KEYXY to bits 12 and 13 of KEYPAD_BITS. KEY_LID needs
    // to be flipped.
    uint16_t keys_arm7_touch_lid = ((keyxy << 6) & (KEY_TOUCH | KEY_LID)) ^ KEY_LID;

    return keys_arm9 | keys_arm7_xy | keys_arm7_debug | keys_arm7_touch_lid;
}

static uint16_t keys = 0;
static uint16_t keysold = 0;
static uint16_t keysrepeat = 0;

static u8 delay = 30, repeat = 15, count = 30;

void scanKeys(void)
{
    keysold = keys;
    keys = keys_cur();

    if (delay != 0)
    {
        if (keys != keysold)
        {
            count = delay;
            keysrepeat = keysDown();
        }
        count--;
        if (count == 0)
        {
            count = repeat;
            keysrepeat = keys;
        }
    }
}

uint32_t keysHeld(void)
{
    return keys;
}

uint32_t keysDown(void)
{
    return (keys & ~keysold);
}

uint32_t keysDownRepeat(void)
{
    uint32_t tmp = keysrepeat;

    keysrepeat = 0;

    return tmp;
}

void keysSetRepeat(u8 setDelay, u8 setRepeat)
{
    delay = setDelay;
    repeat = setRepeat;
    count = delay;
    keysrepeat = 0;
}

uint32_t keysUp(void)
{
    return (keys ^ keysold) & (~keys);
}

uint32_t keysCurrent(void)
{
    return keys_cur();
}
