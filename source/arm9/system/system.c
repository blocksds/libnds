// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2005 Michael Noland (joat)
// Copyright (C) 2005 Jason Rogers (dovoto)
// Copyright (C) 2005 Dave Murphy (WinterMute)

#include <nds/bios.h>
#include <nds/fifocommon.h>
#include <nds/fifomessages.h>
#include <nds/interrupts.h>
#include <nds/memory.h>
#include <nds/system.h>

#include "arm9/libnds_internal.h"
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
        case SYS_ARM7_CRASH:
        {
            REG_IME = 0;

            __TransferRegion volatile *ipc = __transferRegion();
            ExceptionState *ex = (ExceptionState *)&ipc->exceptionState;
            exceptionStatePrint(ex, "ARM7 Guru Meditation Error");

            while (1)
                swiWaitForVBlank();
        }
        case SYS_ARM7_ASSERTION:
        {
            REG_IME = 0;

            __TransferRegion volatile *ipc = __transferRegion();
            AssertionState *as = (AssertionState *)&ipc->assertionState;

            __sassert(as->file, as->line, as->condition, "ARM7 assertion");

            while (1)
                swiWaitForVBlank();
        }
        case SYS_ARM7_CONSOLE_FLUSH:
            consoleArm7Flush();
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

u32 systemSetBacklightLevel(u32 level)
{
    if (level > 5)
        level = 5;

    fifoSendValue32(FIFO_PM, PM_REQ_BACKLIGHT_LEVEL | level);
    fifoWaitValue32(FIFO_PM);
    return fifoGetValue32(FIFO_PM);
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

void ledBlink(PM_LedStates value)
{
    if (!isDSiMode())
        fifoSendValue32(FIFO_PM, PM_REQ_LED | (value & 3));
}

u32 getBatteryLevel(void)
{
    fifoSendValue32(FIFO_PM, PM_REQ_BATTERY);
    fifoWaitValue32(FIFO_PM);
    return fifoGetValue32(FIFO_PM);
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

bool memBufferIsInMainRam(const void *buffer, size_t size)
{
    extern uint8_t __dtcm_start;

    uintptr_t base = (uintptr_t)buffer;

    if (base < 0x02000000)
        return false;

    if ((base + size) > (uintptr_t)&__dtcm_start)
        return false;

    return true;
}
