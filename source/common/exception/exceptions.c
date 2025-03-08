// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2005 Dave Murphy (WinterMute)
// Copyright (c) 2024 Antonio Niño Díaz

#include <nds/arm9/exceptions.h>
#include <nds/cpu_asm.h>
#include <nds/interrupts.h>
#include <nds/memory.h>
#include <nds/ndstypes.h>
#include <nds/system.h>

void setExceptionHandler(VoidFn handler)
{
    EXCEPTION_VECTOR = enterException;
    exceptionC = handler;
}

// ---------------------------------------

const char *exceptionMsg = NULL;

__attribute__((noreturn)) void libndsCrash(const char *msg)
{
    exceptionMsg = msg;

    // Use an undefined instruction defined by the assembler
    asm volatile("udf #0" ::: "memory");

    while (1);
}

// ---------------------------------------

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
