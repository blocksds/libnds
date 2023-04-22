// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2005 Dave Murphy (WinterMute)

#include <stdio.h>

#include <nds/arm9/background.h>
#include <nds/arm9/console.h>
#include <nds/arm9/exceptions.h>
#include <nds/arm9/video.h>
#include <nds/cpu_asm.h>
#include <nds/memory.h>
#include <nds/ndstypes.h>
#include <nds/system.h>

uint32_t ARMShift(uint32_t value, uint8_t shift)
{
    // No shift at all
    if (shift == 0x0B)
        return value;

    int index;
    if (shift & 0x01)
    {
        // Shift index is a register
        index = exceptionRegisters[(shift >> 4) & 0x0F];
    }
    else
    {
        // Constant shift index
        index = (shift >> 3) & 0x1F;
    }

    switch (shift & 0x06)
    {
        case 0x00:
            // Logical left
            return value << index;

        case 0x02:
            // Logical right
            return value >> index;

        case 0x04:
            // Arithmetic right
            return (int32_t)value >> index;

        case 0x06:
            // Rotate right
            index = index & 0x1F;
            value = (value >> index) | (value << (32 - index));
            return value;
    }

    return value;
}

u32 getExceptionAddress(u32 opcodeAddress, u32 thumbState)
{
    int Rf, Rb, Rd, Rn, Rm;

    if (thumbState)
    {
        // Thumb

        uint16_t opcode = *(uint16_t *)opcodeAddress;

        // ldr r,[pc,###]           01001ddd ffffffff
        // ldr r,[r,r]              0101xx0f ffbbbddd
        // ldrsh                    0101xx1f ffbbbddd
        // ldr r,[r,imm]            011xxfff ffbbbddd
        // ldrh                     1000xfff ffbbbddd
        // ldr r,[sp,###]           1001xddd ffffffff
        // push                     1011x10l llllllll
        // ldm                      1100xbbb llllllll

        if ((opcode & 0xF800) == 0x4800)
        {
            // ldr r,[pc,###]
            s8 offset = opcode & 0xff;
            return exceptionRegisters[15] + offset;
        }
        else if ((opcode & 0xF200) == 0x5000)
        {
            // ldr r,[r,r]
            Rb = (opcode >> 3) & 0x07;
            Rf = (opcode >> 6) & 0x07;
            return exceptionRegisters[Rb] + exceptionRegisters[Rf];
        }
        else if ((opcode & 0xF200) == 0x5200)
        {
            // ldrsh
            Rb = (opcode >> 3) & 0x07;
            Rf = (opcode >> 6) & 0x03;
            return exceptionRegisters[Rb] + exceptionRegisters[Rf];
        }
        else if ((opcode & 0xE000) == 0x6000)
        {
            // ldr r,[r,imm]
            Rb = (opcode >> 3) & 0x07;
            Rf = (opcode >> 6) & 0x1F;
            return exceptionRegisters[Rb] + (Rf << 2);
        }
        else if ((opcode & 0xF000) == 0x8000)
        {
            // ldrh
            Rb = (opcode >> 3) & 0x07;
            Rf = (opcode >> 6) & 0x1F;
            return exceptionRegisters[Rb] + (Rf << 2);
        }
        else if ((opcode & 0xF000) == 0x9000)
        {
            // ldr r,[sp,#imm]
            s8 offset = opcode & 0xff;
            return exceptionRegisters[13] + offset;
        }
        else if ((opcode & 0xF700) == 0xB500)
        {
            // push/pop
            return exceptionRegisters[13];
        }
        else if ((opcode & 0xF000) == 0xC000)
        {
            // ldm/stm
            Rd = (opcode >> 8) & 0x07;
            return exceptionRegisters[Rd];
        }
    }
    else
    {
        // ARM32
        uint32_t opcode = *(uint32_t *)opcodeAddress;

        // SWP          xxxx0001 0x00nnnn dddd0000 1001mmmm
        // STR/LDR      xxxx01xx xxxxnnnn ddddffff ffffffff
        // STRH/LDRH    xxxx000x x0xxnnnn dddd0000 1xx1mmmm
        // STRH/LDRH    xxxx000x x1xxnnnn ddddffff 1xx1ffff
        // STM/LDM      xxxx100x xxxxnnnn llllllll llllllll

        if ((opcode & 0x0FB00FF0) == 0x01000090)
        {
            // SWP
            Rn = (opcode >> 16) & 0x0F;
            return exceptionRegisters[Rn];
        }
        else if ((opcode & 0x0C000000) == 0x04000000)
        {
            // STR/LDR
            Rn = (opcode >> 16) & 0x0F;
            if (opcode & 0x02000000)
            {
                // Register offset
                Rm = opcode & 0x0F;
                if (opcode & 0x01000000)
                {
                    uint8_t shift = (opcode >> 4) & 0xFF;
                    // pre indexing
                    int32_t offset = ARMShift(exceptionRegisters[Rm], shift);
                    // add or sub the offset depending on the U-Bit
                    return exceptionRegisters[Rn]
                           + ((opcode & 0x00800000) ? offset : -offset);
                }
                else
                {
                    // Post indexing
                    return exceptionRegisters[Rn];
                }
            }
            else
            {
                // Immediate offset
                uint32_t offset = (opcode & 0xFFF);
                if (opcode & 0x01000000)
                {
                    // Pre indexing

                    // Add or sub the offset depending on the U-Bit
                    return exceptionRegisters[Rn]
                           + ((opcode & 0x00800000) ? offset : -offset);
                }
                else
                {
                    // Post indexing
                    return exceptionRegisters[Rn];
                }
            }
        }
        else if ((opcode & 0x0E400F90) == 0x00000090)
        {
            // LDRH/STRH with register Rm
            Rn = (opcode >> 16) & 0x0F;
            Rd = (opcode >> 12) & 0x0F;
            Rm = opcode & 0x0F;
            uint8_t shift = (opcode >> 4) & 0xFF;
            int32_t offset = ARMShift(exceptionRegisters[Rm], shift);
            // Add or sub the offset depending on the U-Bit
            return exceptionRegisters[Rn] + ((opcode & 0x00800000) ? offset : -offset);
        }
        else if ((opcode & 0x0E400F90) == 0x00400090)
        {
            // LDRH/STRH with immediate offset
            Rn = (opcode >> 16) & 0x0F;
            Rd = (opcode >> 12) & 0x0F;
            uint32_t offset = (opcode & 0xF) | ((opcode & 0xF00) >> 8);
            // Add or sub the offset depending on the U-Bit
            return exceptionRegisters[Rn] + ((opcode & 0x00800000) ? offset : -offset);
        }
        else if ((opcode & 0x0E000000) == 0x08000000)
        {
            // LDM/STM
            Rn = (opcode >> 16) & 0x0F;
            return exceptionRegisters[Rn];
        }
    }
    return 0;
}

static const char *registerNames[] = {
    "r0",  "r1",  "r2",  "r3",  "r4",  "r5", "r6",  "r7",
    "r8 ", "r9 ", "r10", "r11", "r12", "sp ", "lr ", "pc "
};

// Symbol defined in the linkerscript
extern const char __itcm_start[];

void guruMeditationDump(void)
{
    consoleDemoInit();

    // White text on a red background
    BG_PALETTE_SUB[0] = RGB15(31, 0, 0);
    BG_PALETTE_SUB[255] = RGB15(31, 31, 31);

    printf("\x1b[5CGuru Meditation Error!\n");

    // The current CPU mode specifies whether the exception was caused by a data
    // abort or an undefined instruction.
    u32 currentMode = getCPSR() & CPSR_MODE_MASK;

    // Check the location where the BIOS stored the CPSR state at the moment of
    // the exception.
    u32 thumbState = *(EXCEPTION_STACK_TOP - 3) & CPSR_FLAG_T;

    u32 codeAddress, exceptionAddress = 0;

    int offset = 8;

    if (currentMode == CPSR_MODE_ABORT)
    {
        printf("\x1b[10Cdata abort!\n\n");

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

        bool is_itcm = codeAddress > (u32)__itcm_start
                     && codeAddress < (u32)(__itcm_start + 32768);

        // If it's in a code region, try to decode the instruction. This will
        // let us know exactly which address was trying to be accessed.

        if (is_main_ram || is_itcm)
            exceptionAddress = getExceptionAddress(codeAddress, thumbState);
        else
            exceptionAddress = codeAddress;
    }
    else
    {
        printf("\x1b[5Cundefined instruction!\n\n");

        // Get the address where the exception was triggered, which is the one
        // that holds the undefined instruction, so it's the same address as the
        // exception address.

        // That PC will have advanced one instruction, so the actual location of
        // the undefined instruction is one instruction before the current PC.
        if (thumbState)
            offset = 2;
        else
            offset = 4;

        codeAddress = exceptionRegisters[15] - offset;
        exceptionAddress = codeAddress;
    }

    // Finally, print everything to the screen

    printf("  pc: %08lX addr: %08lX\n\n", codeAddress, exceptionAddress);

    for (int i = 0; i < 8; i++)
    {
        printf("  %s: %08lX   %s: %08lX\n", registerNames[i], exceptionRegisters[i],
               registerNames[i + 8], exceptionRegisters[i + 8]);
    }

    printf("\n");
    u32 *stack = (u32 *)exceptionRegisters[13];
    for (int i = 0; i < 10; i++)
    {
        printf("\x1b[%d;2H%08lX:  %08lX %08lX", i + 14, (u32)&stack[i * 2], stack[i * 2],
               stack[(i * 2) + 1]);
    }
}

static void defaultHandler(void)
{
    guruMeditationDump();
    while (1)
        ;
}

void defaultExceptionHandler(void)
{
    setExceptionHandler(defaultHandler);
}
