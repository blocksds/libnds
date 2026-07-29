// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2005 Dave Murphy (WinterMute)
// Copyright (C) 2024-2026 Antonio Niño Díaz

// Exception handlers can't use printf(). printf() uses picolibc locks, which
// use TLS (thread-local storage), so it isn't safe from this file.
//
// The exception handler may run from an IRQ handler, and IRQ handlers aren't
// allowed to use TLS. In libnds debug builds, libndsCrash() is called if this
// situation is detected.

#include <string.h>

#include <nds/arm9/background.h>
#include <nds/arm9/console.h>
#include <nds/arm9/cp15.h>
#include <nds/arm9/video.h>
#include <nds/cpu_asm.h>
#include <nds/interrupts.h>
#include <nds/exceptions.h>

#include "common/libnds_internal.h"

int mpuRegionGet(uintptr_t addr)
{
    // Find which region this address is in. Higher region numbers have a higher
    // priority, so we check them first by flipping the order in the array.

    for (int i = 7; i >= 0; i--)
    {
        uint32_t config = CP15_GetRegion(i);

        if ((config & CP15_CONFIG_REGION_ENABLE) == 0)
            continue;

        uint32_t size_value = config & CP15_CONFIG_REGION_SIZE_MASK;

        // Ignore sizes under 4 KB (they are invalid) and over 256 MB
        // (they aren't useful on DS, so they are most likely a programming
        // error).
        if ((size_value < CP15_REGION_SIZE_4KB) ||
            (size_value >= CP15_REGION_SIZE_256MB))
            continue;

        uint32_t region_size = 1u << (1 + (size_value >> 1));
        uint32_t region_base = config & CP15_CONFIG_REGION_BASE_MASK;
        uint32_t region_end = region_base + region_size - 1;

        if ((region_base <= addr) && (addr <= region_end))
            return i;
    }

    return -1;
}

bool mpuRegionIsCode(int region)
{
    // If this address is outside of all regions, it isn't code
    if ((region < 0) || (region > 7))
        return false;

    // Check if the region we've found is a code or data region
    int instr_permissions = (CP15_GetInstructionPermissions() >> (region * 4)) & 0xF;
    if (instr_permissions != 0x0)
        return true;

    return false;
}

static void exceptionPrintString(const char *msg)
{
    while (1)
    {
        char c = *msg++;
        if (c == '\0')
            break;

        consolePrintChar(c);
    }
}

static void exceptionPrintU32(uint32_t val)
{
    for (int i = 28; i >= 0; i -= 4)
    {
        unsigned int c = (val >> i) & 0xF;

        if (c < 10)
            consolePrintChar(c - 0 + '0');
        else
            consolePrintChar(c - 10 + 'A');
    }
}

void exceptionStatePrint(ExceptionState *ex, const char *title)
{
    consoleDemoInit();

    // White text on a red background
    BG_PALETTE_SUB[0] = RGB15(15, 0, 0);
    BG_PALETTE_SUB[255] = RGB15(31, 31, 31);

    consoleSetCursor(NULL, (32 - strlen(title)) / 2, 0);
    exceptionPrintString(title);

    consoleSetCursor(NULL, (32 - strlen(ex->description)) / 2, 1);
    exceptionPrintString(ex->description);
    exceptionPrintString("\n\n");

    if (true)
    {
        // Finally, print everything to the screen

        exceptionPrintString("  pc: ");
        exceptionPrintU32(ex->reg[15]);
        exceptionPrintString(" addr: ");
        exceptionPrintU32(ex->address);
        exceptionPrintString("\n\n");

        // CPU registers

        for (int i = 0; i < 8; i++)
        {
            const char *registerNames[] =
            {
                "r0",  "r1",  "r2",  "r3",  "r4",  "r5", "r6",  "r7",
                "r8 ", "r9 ", "r10", "r11", "r12", "sp ", "lr ", "pc "
            };

            exceptionPrintString("  ");
            exceptionPrintString(registerNames[i]);
            exceptionPrintString(": ");
            exceptionPrintU32(ex->reg[i]);
            exceptionPrintString("   ");
            exceptionPrintString(registerNames[i + 8]);
            exceptionPrintString(": ");
            exceptionPrintU32(ex->reg[i + 8]);
            exceptionPrintString("\n");
        }

        // Stack dump

        for (int i = 0; i < 10; i++)
        {
            consoleSetCursor(NULL, 2, i + 14);

            exceptionPrintU32(ex->reg[13] + i * 2);
            exceptionPrintString(":  ");
            exceptionPrintU32(ex->stack[i * 2]);
            exceptionPrintString(" ");
            exceptionPrintU32(ex->stack[(i * 2) + 1]);
        }
    }
}

LIBNDS_NORETURN
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
            strcpy(ex.description, "Data abort");

            // In a data abort, there is an instruction that tried to access an
            // invalid address, and an invalid address.

            // Get the address where the exception was triggered

            codeAddress = exceptionRegisters[15] - offset;

            // Check if the address is a region that normally contains code
            int region = mpuRegionIsCode(codeAddress);

            if (mpuRegionIsCode(region))
                exceptionAddress = getExceptionAddress(codeAddress, thumbState);
            else
                exceptionAddress = codeAddress;
        }
        else if (currentMode == CPSR_MODE_UNDEFINED)
        {
            strcpy(ex.description, "Undefined instruction");

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
            strcpy(ex.description, "Unknown error");

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

    exceptionPrintString(msg);

    while (1)
        ;
}

void releaseExceptionHandler(void)
{
    setExceptionHandler(releaseCrashHandler);
}
