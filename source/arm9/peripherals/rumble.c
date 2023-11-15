// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2005 Michael Noland (joat)
// Copyright (C) 2005 Jason Rogers (dovoto)
// Copyright (C) 2005 Dave Murphy (WinterMute)
// Copyright (C) 2005 Mike Parks (BigRedPimp)
// Copyright (c) 2023 Antonio Niño Díaz

#include <nds/arm9/rumble.h>
#include <nds/memory.h>
#include <nds/ndstypes.h>
#include <nds/system.h>

static RUMBLE_TYPE rumbleType = RUMBLE_TYPE_UNKNOWN;

RUMBLE_TYPE rumbleGetType(void)
{
    return rumbleType;
}

void rumbleSetType(RUMBLE_TYPE type)
{
    rumbleType = type;
}

// Definitions for the SuperCard SD Rumble
//
// Note that there is a RAM mode, but the flashcart doesn't actually have a RAM
// chip. Also, note that you shouldn't switch away from mode rumble while the
// motor is running or it will stop.
#define SC_REG_ENABLE       (*(vu16 *)0x9FFFFFE)

#define SC_ENABLE_MAGIC     0xA55A

#define SC_ENABLE_FIRMWARE  (0)
#define SC_ENABLE_RAM       (1 << 0)
#define SC_ENABLE_CARD      (1 << 1)
#define SC_ENABLE_WRITE     (1 << 2) // To be used with SC_ENABLE_RAM
#define SC_ENABLE_RUMBLE    (1 << 3)

static void supercard_set_mode(uint16_t mode)
{
    SC_REG_ENABLE = SC_ENABLE_MAGIC;
    SC_REG_ENABLE = SC_ENABLE_MAGIC;
    SC_REG_ENABLE = mode;
    SC_REG_ENABLE = mode;
}

static bool supercard_rumble_detected(void)
{
    // In firmware mode, the GBA bus should show a valid GBA ROM header

    supercard_set_mode(SC_ENABLE_FIRMWARE);

    if (GBA_HEADER.is96h != 0x96)
        return false;

    if ((GBA_HEADER.gamecode[0] != 'P') || (GBA_HEADER.gamecode[1] != 'A') ||
        (GBA_HEADER.gamecode[2] != 'S') || (GBA_HEADER.gamecode[3] != 'S'))
        return false;

    // In rumble mode, all the GBA bus is mapped to the same register. Bit 1
    // always reads 0, while the other bits seem to read open bus values. Test
    // that at least quite a few of the first addresses return 0 in that bit to
    // make sure that it's not a coincidence.

    supercard_set_mode(SC_ENABLE_RUMBLE);

    for (int i = 0; i < 0x80; i++)
    {
        if (GBA_BUS[i] & BIT(1))
            return false;
    }

    // Remember to keep the rumble mode enabled to control the motor and to keep
    // it running.

    return true;
}

void rumbleInit(void)
{
    // Accessing the slot-2 memory region in DSi mode will cause a MPU
    // exception, so this code can't run on a DSi at all.
    if (isDSiMode())
        return;

    sysSetCartOwner(BUS_OWNER_ARM9);
    rumbleType = RUMBLE_TYPE_NONE;

    if (supercard_rumble_detected())
    {
        rumbleType = RUMBLE_TYPE_SC_RUMBLE;
        return;
    }

    // First, check for 0x96 to see if it's a GBA game
    if (GBA_HEADER.is96h == 0x96)
    {
        // if it is a game, we check the game code
        // to see if it is warioware twisted
        if ((GBA_HEADER.gamecode[0] == 'R') && (GBA_HEADER.gamecode[1] == 'Z')
            && (GBA_HEADER.gamecode[2] == 'W') && (GBA_HEADER.gamecode[3] == 'E'))
        {
            rumbleType = RUMBLE_TYPE_GBA;
            WARIOWARE_ENABLE = 8;
        }
    }
    else
    {
        for (int i = 0; i < 0x1000; i++)
        {
            if (GBA_BUS[i] != (i & 0xFFFD))
                return;
        }
        rumbleType = RUMBLE_TYPE_PAK;
    }
}

bool isRumbleInserted(void)
{
    if (rumbleType == RUMBLE_TYPE_UNKNOWN)
        rumbleInit();

    return rumbleType > RUMBLE_TYPE_NONE;
}

void setRumble(bool position)
{
    // Ensure that the ARM9 owns the bus (in case the user has changed it)
    sysSetCartOwner(BUS_OWNER_ARM9);

    if (rumbleType == RUMBLE_TYPE_GBA)
    {
        WARIOWARE_PAK = position ? 8 : 0;
    }
    else if (rumbleType == RUMBLE_TYPE_PAK)
    {
        RUMBLE_PAK = position ? 2 : 0;
    }
    else if (rumbleType == RUMBLE_TYPE_MAGUKIDDO)
    {
        // TODO: Untested.
        RUMBLE_PAK = position ? 256 : 0;
    }
    else if (rumbleType == RUMBLE_TYPE_SC_RUMBLE)
    {
        GBA_BUS[0] = position ? BIT(1) : 0;
    }
}
