// SPDX-License-Identifier: Zlib
//
// Copyright (c) 2024 Antonio Niño Díaz

#include <stdio.h>

#include <nds/arm7/console.h>
#include <nds/bios.h>
#include <nds/fifocommon.h>

#include "common/libnds_internal.h"

static ConsoleArm7Ipc *con = NULL;

// This is an internal libnds function called by the FIFO handler when the ARM9
// sets up the ARM7 console system.
int consoleSetup(ConsoleArm7Ipc *c)
{
    con = c;

    return 0;
}

bool consoleIsSetup(void)
{
    if (con == NULL)
        return false;

    return true;
}

static uint16_t consoleNextWriteIndex(ConsoleArm7Ipc *c)
{
    if (c->write_index + 1 >= c->buffer_size)
        return 0;
    else
        return c->write_index + 1;
}

bool consoleIsFull(void)
{
    uint16_t next_write_index = consoleNextWriteIndex(con);

    if (next_write_index == con->read_index)
        return true;

    return false;
}

int consolePrintChar(char c)
{
    if (con == NULL)
        return -1;

    if (consoleIsFull())
    {
        consoleFlush();

        do
        {
            // Give some time to the ARM9 to print more than one character so
            // that we don't send too many FIFO commands. It's a lot faster to
            // add characters from the ARM7 than to print them from the ARM9.
            swiDelay(100);
        }
        while (consoleIsFull());
    }

    con->buffer[con->write_index] = c;
    con->write_index++;

    if (con->write_index == con->buffer_size)
        con->write_index = 0;

    return 0;
}

void consoleFlush(void)
{
    fifoSendValue32(FIFO_SYSTEM, SYS_ARM7_CONSOLE_FLUSH);
}
