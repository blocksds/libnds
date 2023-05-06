// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2005 Michael Noland (joat)
// Copyright (C) 2005 Jason Rogers (dovoto)
// Copyright (C) 2005 Dave Murphy (WinterMute)
// Copyright (C) 2005 Mike Parks (BigRedPimp)

#include <nds/arm9/rumble.h>
#include <nds/memory.h>
#include <nds/ndstypes.h>

static RUMBLE_TYPE rumbleType = RUMBLE_TYPE_UNKNOWN;

RUMBLE_TYPE rumbleGetType(void)
{
    return rumbleType;
}

void rumbleSetType(RUMBLE_TYPE type)
{
    rumbleType = type;
}

void rumbleInit(void)
{
    sysSetCartOwner(BUS_OWNER_ARM9);
    rumbleType = RUMBLE_TYPE_NONE;

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
}
