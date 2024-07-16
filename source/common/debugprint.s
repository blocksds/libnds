// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2008 Mukunda Johnson (eKid)
// Copyright (C) 2008-2010 Jason Rogers (dovoto)
// Copyright (C) 2008-2010 Dave Murphy (WinterMute)

#include <nds/asminc.h>

    .syntax  unified

    .text
    .thumb


// no$gba debug output function
// params = { string, length }
BEGIN_ASM_FUNC nocashWrite
    b   1f

// no$gba debug output function
// params = { string }
BEGIN_ASM_FUNC nocashMessage

    // copy string into buffer

    movs    r1, #120        // max string length == 120 bytes
1:
    push    {r4}
    ldr     r4, =buffer     // get buffer address
    movs    r2, #0          // r2 = read/write position
3:
    ldrb    r3, [r0, r2]    // load character
    strb    r3, [r4, r2]    // store character
    cmp     r3, #0          // character == NULL?
    beq     4f              // yes, send message
    adds    r2, #1          // increment read/write position
    cmp     r2, r1          // max length == 120
    bne     3b              // loop if < 120 characters

    movs    r0, #0
    strb    r0, [r4, r2]

    // Send message to no$
4:
    mov     r12, r12        // first ID
    b       2f              // skip the text section
    .hword  0x6464          // second ID
    .hword  0               // flags
buffer:
    .space  120             // data
2:
    pop     {r4}

    bx      lr

    .pool
