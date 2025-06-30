// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2025 Antonio Niño Díaz

#include <string.h>

#include <nds/fifocommon.h>
#include <nds/interrupts.h>
#include <nds/ipc.h>

#include "common/fifo_messages_helpers.h"
#include "common/libnds_internal.h"

// This is called by assert() from picolibc
__attribute__((noreturn))
void __assert_func(const char *file, int line, const char *func,
                   const char *failedexpr)
{
    (void) func;

    REG_IME = 0;

    __TransferRegion volatile *ipc = __transferRegion();
    AssertionState *as = (AssertionState *)&ipc->assertionState;

    // Clear everything in case some fields are left empty
    memset(as, 0, sizeof(AssertionState));

    // Copy the assertion condition
    strncpy(as->condition, failedexpr, sizeof(as->condition) - 1);
    as->condition[sizeof(as->condition) - 1] = '\0';

    // Copy the line number
    as->line = line;

    // Copy the assertion file
    strncpy(as->file, file, sizeof(as->file));
    as->condition[sizeof(as->file) - 1] = '\0';

    // We can't trust the FIFO library at this point. The best we can do is wait
    // until the send FIFO isn't full and send a packet writing to the registers
    // themselves. fifoSendValue32(FIFO_SYSTEM, SYS_ARM7_ASSERTION) wouldn't be
    // reliable.

    while (REG_IPC_FIFO_CR & IPC_FIFO_SEND_FULL)
        ;

    REG_IPC_FIFO_TX = fifo_msg_value32_pack(FIFO_SYSTEM, SYS_ARM7_ASSERTION);

    // We can't make any assumption about what happened before an assertion, so
    // it's better to just hang.
    //
    // By disabling interrupts and calling swiWaitForVBlank() we will get into a
    // low power mode while the CPU waits forever.

    while (1)
        swiWaitForVBlank();
}
