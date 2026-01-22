// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2005 Dave Murphy (WinterMute)
// Copyright (C) 2024 Antonio Niño Díaz

#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#include <nds/exceptions.h>
#include <nds/cpu_asm.h>
#include <nds/fifocommon.h>
#include <nds/interrupts.h>
#include <nds/ipc.h>
#include <nds/memory.h>
#include <nds/ndstypes.h>
#include <nds/system.h>

#include "common/fifo_messages_helpers.h"
#include "common/libnds_internal.h"

LIBNDS_NORETURN
void guruMeditationDump(void)
{
    REG_IME = 0;

    __TransferRegion volatile *ipc = __transferRegion();
    ExceptionState *ex = (ExceptionState *)&ipc->exceptionState;

    // Clear everything in case some fields are left empty
    memset(ex, 0, sizeof(ExceptionState));

    // The current CPU mode specifies whether the exception was caused by a data
    // abort or an undefined instruction.
    u32 currentMode = getCPSR() & CPSR_MODE_MASK;

    // Check the location where the BIOS stored the CPSR state at the moment of
    // the exception.
    u32 thumbState = *(EXCEPTION_STACK_TOP - 3) & CPSR_FLAG_T;

    u32 codeAddress = 0, exceptionAddress = 0;

    // TODO: Check that offsets are correct
    int offset = 8;

    bool dump_state = false;

    if (exceptionMsg != NULL)
    {
        strcpy(ex->description, exceptionMsg);

        // This should have happened because of an undefined instruction, get
        // information the same way.
        if (thumbState)
            offset = 2;
        else
            offset = 4;

        codeAddress = exceptionRegisters[15] - offset;
        exceptionAddress = codeAddress;

        dump_state = true;
    }
    else
    {
        if (currentMode == CPSR_MODE_ABORT)
        {
            strcpy(ex->description, "Data abort!");

            // This should never happen on the ARM7.
        }
        else if (currentMode == CPSR_MODE_UNDEFINED)
        {
            strcpy(ex->description, "Undefined instruction");

            // Get the address where the exception was triggered, which is the
            // one that holds the undefined instruction, so it's the same
            // address as the exception address.

            // That PC will have advanced one instruction, so the actual
            // location of the undefined instruction is one instruction before
            // the current PC.
            if (thumbState)
                offset = 2;
            else
                offset = 4;

            codeAddress = exceptionRegisters[15] - offset;
            exceptionAddress = codeAddress;

            dump_state = true;
        }
        else
        {
            strcpy(ex->description, "Unknown error");

            // If we're here because of an unknown error we can't get any useful
            // information.
        }
    }

    if (dump_state)
    {
        // Finally, save everything to IPC memory

        memcpy(&(ex->reg[0]), &(exceptionRegisters[0]), sizeof(ex->reg));

        ex->reg[15] = codeAddress;
        ex->address = exceptionAddress;

        u32 *stack = (u32 *)exceptionRegisters[13];
        for (int i = 0; i < 10; i++)
        {
            ex->stack[(i * 2) + 0] = stack[(i * 2) + 0];
            ex->stack[(i * 2) + 1] = stack[(i * 2) + 1];
        }
    }

    // We can't trust the FIFO library at this point. The best we can do is wait
    // until the send FIFO isn't full and send a packet writing to the registers
    // themselves. fifoSendValue32(FIFO_SYSTEM, SYS_ARM7_CRASH) wouldn't be
    // reliable.

    while (REG_IPC_FIFO_CR & IPC_FIFO_SEND_FULL)
        ;

    REG_IPC_FIFO_TX = fifo_msg_value32_pack(FIFO_SYSTEM, SYS_ARM7_CRASH);

    // We can't make any assumption about what happened before an exception. It
    // may have happened when dereferencing a NULL pointer before doing any
    // harm, or it may happen because of a corrupted return address after a
    // stack overflow.
    //
    // In any case, we can't assume that the exit-to-loader code hasn't been
    // corrupted, so it's a good idea to wait here forever.
    //
    // By disabling interrupts and calling swiWaitForVBlank() we will get into a
    // low power mode while the CPU waits forever.

    while (1)
        swiWaitForVBlank();
}

LIBNDS_NORETURN
static void defaultHandler(void)
{
    guruMeditationDump();
}

void defaultExceptionHandler(void)
{
    setExceptionHandler(defaultHandler);
}

// ---------------------------------------

LIBNDS_NORETURN
static void releaseCrashHandler(void)
{
    REG_IME = 0;

    const char *msg = exceptionMsg;

    if (msg == NULL)
    {
        // If there is no message, try to determine the reason for the crash.
        // The current CPU mode specifies whether the exception was caused by a
        // data abort or an undefined instruction.
        u32 currentMode = getCPSR() & CPSR_MODE_MASK;
        if (currentMode == CPSR_MODE_ABORT)
            msg = "Data abort";
        else if (currentMode == CPSR_MODE_UNDEFINED)
            msg = "Undefined instruction";
        else
            msg = "Unknown error";
    }

    __TransferRegion volatile *ipc = __transferRegion();
    ExceptionState *ex = (ExceptionState *)&(ipc->exceptionState);

    strcpy(ex->description, msg);

    while (REG_IPC_FIFO_CR & IPC_FIFO_SEND_FULL)
        ;

    REG_IPC_FIFO_TX = fifo_msg_value32_pack(FIFO_SYSTEM, SYS_ARM7_CRASH);

    while (1)
        swiWaitForVBlank();
}

void releaseExceptionHandler(void)
{
    setExceptionHandler(releaseCrashHandler);
}
