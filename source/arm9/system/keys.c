// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2005 Christian Auby (DesktopMan)
// Copyright (C) 2005-2008 Dave Murphy (WinterMute)

// Key and touch screen input code.

#include <stdlib.h>

#include <nds/arm9/input.h>
#include <nds/ipc.h>
#include <nds/input.h>
#include <nds/interrupts.h>
#include <nds/system.h>

#include "common/libnds_internal.h"

// This is updated whenever the FIFO handler receives a message from the ARM7
static touchPosition receivedTouchPosition;
static u16 receivedArm7Buttons = 0xFFFF; // Not pressed

// This is updated from receivedTouchPosition and receivedArm7Buttons whenever
// scanKeys() is called.
//
// This prevents a race condition when keysHeld() may say that KEY_TOUCH is
// pressed, but the touch screen status is updated before calling
// touchPosition(), which returns (0, 0) because the player has stopped
// touching the screen.
static touchPosition latchedTouchPosition;
static u16 latchedArm7Buttons = 0xFFFF;

static uint16_t keys_cur(void)
{
    const uint16_t keyinput_mask = KEY_A | KEY_B | KEY_SELECT | KEY_START |
        KEY_RIGHT | KEY_LEFT | KEY_UP | KEY_DOWN | KEY_R | KEY_L;

    uint16_t keyinput = ~REG_KEYINPUT;
    uint16_t keyxy = ~latchedArm7Buttons;

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

static uint16_t keys = 0, keysdown = 0, keysup = 0;
static uint16_t keysold = 0;
static uint16_t keysrepeat = 0;

static u8 delay = 30, repeat = 15, count = 30;

void scanKeys(void)
{
    int oldIME = enterCriticalSection();

    // Get current copy of ARM7 input
    latchedTouchPosition = receivedTouchPosition;
    latchedArm7Buttons = receivedArm7Buttons;

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

    keysdown = keys & ~keysold;
    keysup = (keys ^ keysold) & (~keys);

    leaveCriticalSection(oldIME);
}

uint32_t keysHeld(void)
{
    return keys;
}

uint32_t keysDown(void)
{
    return keysdown;
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
    return keysup;
}

uint32_t keysCurrent(void)
{
    return keys_cur();
}

void touchRead(touchPosition *data)
{
    if (data == NULL)
        return;

    int oldIME = enterCriticalSection();
    *data = latchedTouchPosition;
    leaveCriticalSection(oldIME);
}

void setTransferInputData(touchPosition *touch, u16 buttons)
{
    receivedTouchPosition = *touch;
    receivedArm7Buttons = buttons;
}
