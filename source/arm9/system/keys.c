// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2005 Christian Auby (DesktopMan)
// Copyright (C) 2005 Dave Murphy (WinterMute)

// Key input code. Provides slightly higher level input forming.

#include <stdlib.h>

#include <nds/arm9/input.h>
#include <nds/ipc.h>
#include <nds/system.h>

#include "common/libnds_internal.h"

static inline uint16_t keys_cur(void)
{
    return (((~REG_KEYINPUT) & 0x3ff) | (((~__transferRegion()->buttons) & 3) << 10)
            | (((~__transferRegion()->buttons) << 6) & (KEY_TOUCH | KEY_LID))) ^ KEY_LID;
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
