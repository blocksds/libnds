// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2008-2010 Dave Murphy (WinterMute)
// Copyright (C) 2008 Jason Rogers (Dovoto)

#include <nds/arm7/input.h>
#include <nds/arm7/serial.h>
#include <nds/arm7/touch.h>
#include <nds/fifocommon.h>
#include <nds/fifomessages.h>
#include <nds/ipc.h>
#include <nds/ndstypes.h>
#include <nds/system.h>
#include <nds/touch.h>

static u16 sleepCounter = 0;
static u16 sleepCounterMax = 20;

void inputSetLidSleepDuration(u16 frames)
{
    sleepCounterMax = frames;
}

void inputGetAndSend(void)
{
    touchPosition tempPos = {0};
    FifoMessage msg = {0};

    u16 keys = REG_KEYXY;

    // touchPenDown() handles DSi-mode touch detection
    // (on DS mode, it just checks REG_KEYXY & KEYXY_TOUCH)
    if (!touchPenDown())
        keys |= KEYXY_TOUCH;
    else
        keys &= ~KEYXY_TOUCH;

    msg.SystemInput.keys = keys;

    if (!(keys & KEYXY_TOUCH))
    {
        // only mark pen as down if valid coordinates
        msg.SystemInput.keys |= KEYXY_TOUCH;

        touchReadXY(&tempPos);

        if (tempPos.rawx && tempPos.rawy)
        {
            msg.SystemInput.keys &= ~KEYXY_TOUCH;
            msg.SystemInput.touch = tempPos;
        }
    }

    // Sleep if lid has been closed for a specified number of frames
    if (sleepCounterMax > 0)
    {
        if (keys & KEYXY_LID)
            sleepCounter++;
        else
            sleepCounter = 0;

        if (sleepCounter >= sleepCounterMax)
        {
            systemSleep();
            sleepCounter = 0;
        }
    }

    msg.type = SYS_INPUT_MESSAGE;
    fifoSendDatamsg(FIFO_SYSTEM, sizeof(msg), (u8 *)&msg);
}
