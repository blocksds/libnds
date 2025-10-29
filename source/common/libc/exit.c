// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2009-2012 Dave Murphy (WinterMute)
// Copyright (C) 2023 Antonio Niño Díaz

#include <stdlib.h>
#include <nds/exceptions.h>
#include <nds/fifocommon.h>
#include <nds/system.h>

#ifdef ARM9
#include <nds/arm9/cp15.h>
#include "arm9/libnds_internal.h"
#else
#include "arm7/libnds_internal.h"
#endif
#include "common/fifo_messages_helpers.h"
#include "common/libnds_internal.h"

extern char *fake_heap_end;

// Weak symbol allowing catching non-zero exits from main().

ARM_CODE void __attribute__((weak)) systemErrorExit(int rc)
{
    (void)rc;
}

// System exit is performed as follows:
//
//          main() -> returns rc
//                  V
// __libnds_exit() -> called by crt0
//                  V
//          exit() -> calls atexit() handlers
//                  V
//         _exit() -> returns to loader/shuts down system

ARM_CODE void __libnds_exit(int rc)
{
    exit(rc);
}

ARM_CODE void _exit(int rc)
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
        CP15_MPUDisable();

        if (isDSiMode())
        {
            // Restore extended DSi RAM size to 32 MB to prevent crashes with
            // loaders that incorrectly use REG_SCFG_EXT to determine the size
            // of RAM.
            REG_SCFG_EXT |= SCFG_EXT_RAM_DEBUG | SCFG_EXT_RAM_TWL;
        }

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
        systemReboot();
        systemShutDown();
    }

    while (1);
}

// As this file is always linked in by the crt0, it makes for a good place
// to include newlib/picolibc stack smash protection overrides.
uintptr_t __stack_chk_guard = 0x00000aff;

__attribute__((noreturn))
THUMB_CODE void __stack_chk_fail(void)
{
    libndsCrash("Stack corrupted");
}
