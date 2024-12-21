// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2005 Dave Murphy (WinterMute)
// Copyright (c) 2024 Antonio Niño Díaz

#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#include <nds/arm9/background.h>
#include <nds/arm9/console.h>
#include <nds/arm9/video.h>
#include <nds/cpu_asm.h>
#include <nds/interrupts.h>
#include <nds/exceptions.h>
#include <nds/memory.h>
#include <nds/ndstypes.h>
#include <nds/system.h>

#include "common/libnds_internal.h"

void exceptionStatePrint(ExceptionState *ex, const char *title)
{
    consoleDemoInit();

    // White text on a red background
    BG_PALETTE_SUB[0] = RGB15(15, 0, 0);
    BG_PALETTE_SUB[255] = RGB15(31, 31, 31);

    consoleSetCursor(NULL, (32 - strlen(title)) / 2, 0);
    printf(title);

    consoleSetCursor(NULL, (32 - strlen(ex->description)) / 2, 1);
    printf("%s\n\n", ex->description);

    if (true)
    {
        // Finally, print everything to the screen

        printf("  pc: %08" PRIX32 " addr: %08" PRIX32 "\n\n", ex->reg[15], ex->address);

        for (int i = 0; i < 8; i++)
        {
            const char *registerNames[] =
            {
                "r0",  "r1",  "r2",  "r3",  "r4",  "r5", "r6",  "r7",
                "r8 ", "r9 ", "r10", "r11", "r12", "sp ", "lr ", "pc "
            };

            printf("  %s: %08" PRIX32 "   %s: %08" PRIX32 "\n",
                   registerNames[i], ex->reg[i],
                   registerNames[i + 8], ex->reg[i + 8]);
        }

        printf("\n");
        for (int i = 0; i < 10; i++)
        {
            consoleSetCursor(NULL, 2, i + 14);
            printf("%08" PRIX32 ":  %08" PRIX32 " %08" PRIX32 "",
                   (u32)(ex->reg[13] + i * 2), ex->stack[i * 2], ex->stack[(i * 2) + 1]);
        }
    }
}

__attribute__((noreturn))
void guruMeditationDump(void)
{
    REG_IME = 0;

    ExceptionState ex = { 0 };

    // The current CPU mode specifies whether the exception was caused by a data
    // abort or an undefined instruction.
    u32 currentMode = getCPSR() & CPSR_MODE_MASK;

    // Check the location where the BIOS stored the CPSR state at the moment of
    // the exception.
    u32 thumbState = *(EXCEPTION_STACK_TOP - 3) & CPSR_FLAG_T;

    u32 codeAddress = 0, exceptionAddress = 0;

    int offset = 8;

    bool print_information = true;

    if (exceptionMsg != NULL)
    {
        size_t tab = (32 - strlen(exceptionMsg)) / 2;
        if (tab > 16)
            tab = 0;
        consoleSetCursor(NULL, tab, 1);
        strcpy(ex.description, exceptionMsg);

        // This should have happened because of an undefined instruction, get
        // information the same way.
        if (thumbState)
            offset = 2;
        else
            offset = 4;

        codeAddress = exceptionRegisters[15] - offset;
        exceptionAddress = codeAddress;
    }
    else
    {
        if (currentMode == CPSR_MODE_ABORT)
        {
            strcpy(ex.description, "Data abort!");

            // In a data abort, there is an instruction that tried to access an
            // invalid address, and an invalid address.

            // Get the address where the exception was triggered

            codeAddress = exceptionRegisters[15] - offset;

            // Check if the address is a region that normally contains code

            // TODO: Support DS debugger regions?
            bool is_main_ram;
            if (isDSiMode())
                is_main_ram = codeAddress > 0x02000000 && codeAddress < 0x03000000;
            else
                is_main_ram = codeAddress > 0x02000000 && codeAddress < 0x02400000;

            // Symbol defined in the linkerscript
            extern const char __itcm_start[];

            bool is_itcm = codeAddress > (u32)__itcm_start
                        && codeAddress < (u32)(__itcm_start + 32768);

            // If it's in a code region, try to decode the instruction. This
            // will let us know exactly which address was trying to be accessed.

            if (is_main_ram || is_itcm)
                exceptionAddress = getExceptionAddress(codeAddress, thumbState);
            else
                exceptionAddress = codeAddress;
        }
        else if (currentMode == CPSR_MODE_UNDEFINED)
        {
            strcpy(ex.description, "Undefined instruction!");

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
        }
        else
        {
            strcpy(ex.description, "Unknown error!");

            // If we're here because of an unknown error we can't print anything
            print_information = false;
        }
    }

    if (print_information)
    {
        memcpy(&(ex.reg[0]), &(exceptionRegisters[0]), sizeof(ex.reg));

        ex.reg[15] = codeAddress;
        ex.address = exceptionAddress;

        u32 *stack = (u32 *)exceptionRegisters[13];
        for (int i = 0; i < 10; i++)
        {
            ex.stack[(i * 2) + 0] = stack[(i * 2) + 0];
            ex.stack[(i * 2) + 1] = stack[(i * 2) + 1];
        }
    }

    exceptionStatePrint(&ex, "ARM9 Guru Meditation Error");

    // We can't make any assumption about what happened before an exception. It
    // may have happened when dereferencing a NULL pointer before doing any
    // harm, or it may happen because of a corrupted return address after a
    // stack overflow.
    //
    // In any case, we can't assume that the exit-to-loader code hasn't been
    // corrupted, so it's a good idea to wait here forever.

    while (1)
        ;
}

__attribute__((noreturn))
static void defaultHandler(void)
{
    guruMeditationDump();
}

void defaultExceptionHandler(void)
{
    setExceptionHandler(defaultHandler);
}

// ---------------------------------------

__attribute__((noreturn))
static void releaseCrashHandler(void)
{
    REG_IME = 0;

    consoleDemoInit();

    // White text on a red background
    BG_PALETTE_SUB[0] = RGB15(15, 0, 0);
    BG_PALETTE_SUB[255] = RGB15(31, 31, 31);

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

    while (1)
    {
        char c = *msg++;
        if (c == '\0')
            break;

        consolePrintChar(c);
    }

    while (1)
        ;
}

void releaseExceptionHandler(void)
{
    setExceptionHandler(releaseCrashHandler);
}
