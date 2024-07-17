// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2005 Michael Noland (joat)
// Copyright (C) 2005 Jason Rogers (dovoto)
// Copyright (C) 2005 Dave Murphy (WinterMute)

#include <nds/arm9/sassert.h>
#include <nds/bios.h>
#include <nds/fifocommon.h>
#include <nds/fifomessages.h>
#include <nds/interrupts.h>
#include <nds/memory.h>
#include <nds/system.h>

#include "common/libnds_internal.h"

static void (*SDcallback)(int) = NULL;

void setSDcallback(void (*callback)(int))
{
    SDcallback = callback;
}

// Handle system requests from the ARM7
void systemValueHandler(u32 value, void *data)
{
    (void)data;

    switch (value)
    {
        case PM_REQ_SLEEP:
            systemSleep();
            break;
        case SDMMC_INSERT:
            if (SDcallback)
                SDcallback(1);
            break;
        case SDMMC_REMOVE:
            if (SDcallback)
                SDcallback(0);
            break;
    }
}

void systemMsgHandler(int bytes, void *user_data)
{
    (void)user_data;

    FifoMessage msg;

    fifoGetDatamsg(FIFO_SYSTEM, bytes, (u8 *)&msg);

    switch (msg.type)
    {
        case SYS_INPUT_MESSAGE:
            setTransferInputData(&(msg.SystemInput.touch), msg.SystemInput.keys);
            break;
    }
}

void systemSleep(void)
{
    fifoSendValue32(FIFO_PM, PM_REQ_SLEEP);

    // 100ms
    swiDelay(419000);
}

void enableSleep(void)
{
    fifoSendValue32(FIFO_PM, PM_REQ_SLEEP_ENABLE);
}

void disableSleep(void)
{
    fifoSendValue32(FIFO_PM, PM_REQ_SLEEP_DISABLE);
}

void powerOn(uint32_t bits)
{
    // If the flag PM_ARM9_DIRECT is set, this is a direct write to the ARM9
    // REG_POWERCNT register. If not, send the message to the ARM7 to do it.
    if (bits & PM_ARM9_DIRECT)
        REG_POWERCNT |= bits & 0xFFFF;
    else
        fifoSendValue32(FIFO_PM, PM_REQ_ON | (bits & 0xFFFF));
}

void powerOff(uint32_t bits)
{
    // If the flag PM_ARM9_DIRECT is set, this is a direct write to the ARM9
    // REG_POWERCNT register. If not, send the message to the ARM7 to do it.
    if (bits & PM_ARM9_DIRECT)
        REG_POWERCNT &= ~(bits & 0xFFFF);
    else
        fifoSendValue32(FIFO_PM, PM_REQ_OFF | (bits & 0xFFFF));
}

void ledBlink(int bm)
{
    fifoSendValue32(FIFO_PM, PM_REQ_LED | bm);
}

u32 getBatteryLevel(void)
{
    fifoSendValue32(FIFO_PM, PM_REQ_BATTERY);
    fifoWaitValue32(FIFO_PM);
    return fifoGetValue32(FIFO_PM);
}

void waitARM7Ready(void)
{
    // The ARM9 main() routine should never start before the FIFO handlers of
    // the ARM7 are ready.
    //
    // This function simply waits for the ARM7 to have the handlers setup.
    // Even if the ARM7 FIFO handlers aren't ready when this message is sent,
    // the ARM7 can't clear the send FIFO of the ARM9, so this message will get
    // stuck in the send FIFO for as long as it has to. As soon as the ARM7 has
    // finished setting up the handlers, it will be handled.
    //
    // It is possible that the ARM7 crashes at some point during boot. In order
    // to not wait forever, there is a limit in the number of frames that we're
    // allowed to wait. After that, we will show an error message.

    int max_frames = 60;

    fifoSendValue32(FIFO_PM, PM_IS_ARM7_READY);
    while (1)
    {
        u32 value = fifoGetValue32(FIFO_PM);
        if (value == LIBNDS_ARM7_READY_MAGIC)
            break;

        max_frames--;
        if (max_frames == 0)
        {
            libndsCrash("ARM7 FIFO init failed");
            break;
        }

        swiWaitForVBlank();
    }
}

void enableSlot1(void)
{
    if (isDSiMode())
        fifoSendValue32(FIFO_PM, PM_REQ_SLOT1_ENABLE);
}

void disableSlot1(void)
{
    if (isDSiMode())
        fifoSendValue32(FIFO_PM, PM_REQ_SLOT1_DISABLE);
}
