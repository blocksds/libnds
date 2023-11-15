// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2009-2012 Dave Murphy (WinterMute)
// Copyright (C) 2023 Antonio Niño Díaz

#include <nds/fifocommon.h>
#include <nds/system.h>

#include "fifo_ipc_messages.h"
#include "libnds_internal.h"

extern char *fake_heap_end;

ARM_CODE void __attribute__((weak)) systemErrorExit(int rc)
{
    (void)rc;
}

ARM_CODE void __libnds_exit(int rc)
{
    if (rc != 0)
        systemErrorExit(rc);

    struct __bootstub *bootcode = __transferRegion()->bootcode;

    if (bootcode->bootsig == BOOTSIG)
    {
        // Both CPUs need to be running for a reset to be possible. It doesn't
        // matter if the ARM7 initiates it or if it's done by the ARM9.
        //
        // For example, in NDS Homebrew Menu:
        //
        // - ARM9-initiated reset:
        //   - ARM9 loads the loader code to VRAM_C, which is ARM7 code.
        //   - ARM9 makes the ARM7 jump to VRAM_C.
        //   - ARM9 enters an infinite loop waiting for a start address.
        //   - The loader code runs from the ARM7 and loads a NDS ROM.
        //   - ARM7 tells the start address to the ARM9 of the ROM.
        //   - ARM7 jumps to the start address of the ARM7 of the ROM.
        //
        // - ARM7-initiated reset:
        //   - ARM7 makes the ARM9 jump to the exit vector.
        //   - ARM7 enters an infinite loop.
        //   - An ARM9-initiated reset starts
        //
        // The ARM7-initiated reset is redundant because it doesn't work as an
        // emergency exit in case the ARM9 has crashed. If the ARM9 has crashed
        // enough to not receive a FIFO message from the ARM7, there is no way
        // they can sync enough to do a successful exit.
#ifdef ARM9
        bootcode->arm9reboot();
#endif
#ifdef ARM7
        //bootcode->arm7reboot();

        // Send a special command to the ARM9 to initiate a reset.
        fifoSendSpecialCommand(FIFO_ARM7_REQUESTS_ARM9_RESET);
#endif
    }
    else
    {
        systemShutDown();
    }

    while (1);
}
