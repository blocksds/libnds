// SPDX-License-Identifier: BSD-3-Clause
//
// Copyright (C) 2006 Michael "Chishm" Chisholm

// Interface to the low level disc functions. Used by the higher level file
// system code. Based on code originally written by MightyMax.

#include <stdlib.h>
#include <string.h>
#include <sys/fcntl.h>
#include <unistd.h>

#include <nds/arm9/cache.h>
#include <nds/arm9/dldi.h>
#include <nds/fifocommon.h>
#include <nds/fifomessages.h>
#include <nds/memory.h>
#include <nds/system.h>

const u32 DLDI_MAGIC_NUMBER = 0xBF8DA5ED;

// Stored backwards to prevent it being picked up by DLDI patchers
const char DLDI_MAGIC_STRING_BACKWARDS[DLDI_MAGIC_STRING_LEN] = {
    '\0', 'm', 'h', 's', 'i',  'h', 'C', ' '
};

// The only built in driver
extern DLDI_INTERFACE _io_dldi_stub;
extern char __dldi_end;

const DLDI_INTERFACE *io_dldi_data = &_io_dldi_stub;

// -----------------------------------------------------------------------------

bool dldi_arm7_startup(void)
{
    FifoMessage msg;
    msg.type = DLDI_STARTUP;
    msg.dldiStartupParams.io_interface = &_io_dldi_stub.ioInterface;

    fifoMutexAcquire(FIFO_STORAGE);

    fifoSendDatamsg(FIFO_STORAGE, sizeof(msg), (u8 *)&msg);
    fifoWaitValue32Async(FIFO_STORAGE);
    int result = fifoGetValue32(FIFO_STORAGE);

    fifoMutexRelease(FIFO_STORAGE);

    return result != 0;
}

bool dldi_arm7_is_inserted(void)
{
    fifoMutexAcquire(FIFO_STORAGE);

    fifoSendValue32(FIFO_STORAGE, DLDI_IS_INSERTED);
    fifoWaitValue32Async(FIFO_STORAGE);
    int result = fifoGetValue32(FIFO_STORAGE);

    fifoMutexRelease(FIFO_STORAGE);

    return result != 0;
}

bool dldi_arm7_read_sectors(sec_t sector, sec_t numSectors, void *buffer)
{
    DC_FlushRange(buffer, numSectors * 512);

    FifoMessage msg;
    msg.type = DLDI_READ_SECTORS;
    msg.sdParams.startsector = sector;
    msg.sdParams.numsectors = numSectors;
    msg.sdParams.buffer = buffer;

    fifoMutexAcquire(FIFO_STORAGE);

    fifoSendDatamsg(FIFO_STORAGE, sizeof(msg), (u8 *)&msg);

    fifoWaitValue32Async(FIFO_STORAGE);
    DC_InvalidateRange(buffer, numSectors * 512);

    int result = fifoGetValue32(FIFO_STORAGE);

    fifoMutexRelease(FIFO_STORAGE);

    return result != 0;
}

bool dldi_arm7_write_sectors(sec_t sector, sec_t numSectors, const void *buffer)
{
    DC_FlushRange(buffer, numSectors * 512);

    FifoMessage msg;
    msg.type = DLDI_WRITE_SECTORS;
    msg.sdParams.startsector = sector;
    msg.sdParams.numsectors = numSectors;
    msg.sdParams.buffer = (void *)buffer;

    fifoMutexAcquire(FIFO_STORAGE);

    fifoSendDatamsg(FIFO_STORAGE, sizeof(msg), (u8 *)&msg);

    fifoWaitValue32Async(FIFO_STORAGE);
    DC_InvalidateRange(buffer, numSectors * 512);

    int result = fifoGetValue32(FIFO_STORAGE);

    fifoMutexRelease(FIFO_STORAGE);

    return result != 0;
}

bool dldi_arm7_clear_status(void)
{
    fifoMutexAcquire(FIFO_STORAGE);

    fifoSendValue32(FIFO_STORAGE, DLDI_CLEAR_STATUS);
    fifoWaitValue32Async(FIFO_STORAGE);
    int result = fifoGetValue32(FIFO_STORAGE);

    fifoMutexRelease(FIFO_STORAGE);

    return result != 0;
}

bool dldi_arm7_shutdown(void)
{
    fifoMutexAcquire(FIFO_STORAGE);

    fifoSendValue32(FIFO_STORAGE, DLDI_SHUTDOWN);
    fifoWaitValue32Async(FIFO_STORAGE);
    int result = fifoGetValue32(FIFO_STORAGE);

    fifoMutexRelease(FIFO_STORAGE);

    return result != 0;
}

// Driver that sends commands to the ARM7 to perform operations
DISC_INTERFACE __io_dldi_arm7_interface = {
    0, // Filled at runtime
    0, // Filled at runtime
    &dldi_arm7_startup,
    &dldi_arm7_is_inserted,
    &dldi_arm7_read_sectors,
    &dldi_arm7_write_sectors,
    &dldi_arm7_clear_status,
    &dldi_arm7_shutdown
};

// -----------------------------------------------------------------------------

static DLDI_MODE dldi_mode = DLDI_MODE_AUTODETECT;

void dldiSetMode(DLDI_MODE mode)
{
    dldi_mode = mode;
}

DLDI_MODE dldiGetMode(void)
{
    return dldi_mode;
}

const DISC_INTERFACE *dldiGetInternal(void)
{
    if (dldi_mode == DLDI_MODE_AUTODETECT)
    {
        if (_io_dldi_stub.ioInterface.features & FEATURE_ARM7_CAPABLE)
            dldi_mode = DLDI_MODE_ARM7;
        else
            dldi_mode = DLDI_MODE_ARM9;
    }

    int bus_owner;
    const DISC_INTERFACE *interface = NULL;

    if (dldi_mode == DLDI_MODE_ARM7)
    {
        bus_owner = BUS_OWNER_ARM7;

        __io_dldi_arm7_interface.ioType = _io_dldi_stub.ioInterface.ioType;
        __io_dldi_arm7_interface.features = _io_dldi_stub.ioInterface.features;

        interface = &__io_dldi_arm7_interface;
    }
    else
    {
        bus_owner = BUS_OWNER_ARM9;
        interface = &_io_dldi_stub.ioInterface;
    }

    // If this is a slot-2 flashcart, set the owner of slot-2
    if (_io_dldi_stub.ioInterface.features & FEATURE_SLOT_GBA)
        sysSetCartOwner(bus_owner);

    // If this is a slot-1 flashcard, set the owner of slot-1
    if (_io_dldi_stub.ioInterface.features & FEATURE_SLOT_NDS)
        sysSetCardOwner(bus_owner);

    return interface;
}

bool dldiIsValid(const DLDI_INTERFACE *io)
{
    if (io->magicNumber != DLDI_MAGIC_NUMBER)
        return false;

    for (int i = 0; i < DLDI_MAGIC_STRING_LEN; i++)
    {
        if (io->magicString[i]
            != DLDI_MAGIC_STRING_BACKWARDS[DLDI_MAGIC_STRING_LEN - 1 - i])
        {
            return false;
        }
    }

    return true;
}

void* dldiGetStubDataEnd(void)
{
    // Filter out invalid BSS pointers.
    return ((uintptr_t)_io_dldi_stub.bssEnd) < 0x10000000 && _io_dldi_stub.bssEnd > _io_dldi_stub.dldiEnd
        ? _io_dldi_stub.bssEnd
        : _io_dldi_stub.dldiEnd;
}

void* dldiGetStubEnd(void)
{
    return &__dldi_end;
}

void dldiFixDriverAddresses(DLDI_INTERFACE *io)
{
    u32 offset;
    u8 **address;
    u8 *oldStart;
    u8 *oldEnd;

    offset = (char *)io - (char *)(io->dldiStart);

    oldStart = io->dldiStart;
    oldEnd = io->dldiEnd;

    // Correct all pointers to the offsets from the location of this interface
    io->dldiStart = (char *)io->dldiStart + offset;
    io->dldiEnd = (char *)io->dldiEnd + offset;
    io->interworkStart = (char *)io->interworkStart + offset;
    io->interworkEnd = (char *)io->interworkEnd + offset;
    io->gotStart = (char *)io->gotStart + offset;
    io->gotEnd = (char *)io->gotEnd + offset;
    io->bssStart = (char *)io->bssStart + offset;
    io->bssEnd = (char *)io->bssEnd + offset;

    io->ioInterface.startup =
        (FN_MEDIUM_STARTUP)((intptr_t)io->ioInterface.startup + offset);
    io->ioInterface.isInserted =
        (FN_MEDIUM_ISINSERTED)((intptr_t)io->ioInterface.isInserted + offset);
    io->ioInterface.readSectors =
        (FN_MEDIUM_READSECTORS)((intptr_t)io->ioInterface.readSectors + offset);
    io->ioInterface.writeSectors =
        (FN_MEDIUM_WRITESECTORS)((intptr_t)io->ioInterface.writeSectors + offset);
    io->ioInterface.clearStatus =
        (FN_MEDIUM_CLEARSTATUS)((intptr_t)io->ioInterface.clearStatus + offset);
    io->ioInterface.shutdown =
        (FN_MEDIUM_SHUTDOWN)((intptr_t)io->ioInterface.shutdown + offset);

    // Fix all addresses with in the DLDI
    if (io->fixSectionsFlags & FIX_ALL)
    {
        for (address = (u8 **)io->dldiStart; address < (u8 **)io->dldiEnd; address++)
        {
            if (oldStart <= *address && *address < oldEnd)
                *address += offset;
        }
    }

    // Fix the interworking glue section
    if (io->fixSectionsFlags & FIX_GLUE)
    {
        address = (u8 **)io->interworkStart;

        for (; address < (u8 **)io->interworkEnd; address++)
        {
            if (oldStart <= *address && *address < oldEnd)
                *address += offset;
        }
    }

    // Fix the global offset table section
    if (io->fixSectionsFlags & FIX_GOT)
    {
        for (address = (u8 **)io->gotStart; address < (u8 **)io->gotEnd; address++)
        {
            if (oldStart <= *address && *address < oldEnd)
                *address += offset;
        }
    }

    // Initialise the BSS to 0
    if (io->fixSectionsFlags & FIX_BSS)
        memset(io->bssStart, 0, (u8 *)io->bssEnd - (u8 *)io->bssStart);
}

DLDI_INTERFACE *dldiLoadFromFile(const char *path)
{
    DLDI_INTERFACE *device;
    int fd;
    size_t dldiSize;

    // Read in the DLDI header
    if ((fd = open(path, O_RDONLY, 0)) < 0)
        return NULL;

    const ssize_t dldi_interface_size = sizeof(DLDI_INTERFACE);

    if ((device = malloc(dldi_interface_size)) == NULL)
    {
        close(fd);
        return NULL;
    }

    if (read(fd, device, dldi_interface_size) < dldi_interface_size)
    {
        free(device);
        close(fd);
        return NULL;
    }

    // Check that it is a valid DLDI
    if (!dldiIsValid(device))
    {
        free(device);
        close(fd);
        return NULL;
    }

    // Calculate actual size of DLDI

    // Although the file may only go to the dldiEnd, the BSS section can extend
    // past that. Many DLDI files which don't use BSS set the value to 0.
    if (device->bssEnd && device->dldiEnd > device->bssEnd)
        dldiSize = (char *)device->dldiEnd - (char *)device->dldiStart;
    else
        dldiSize = (char *)device->bssEnd - (char *)device->dldiStart;

    dldiSize = (dldiSize + 0x03) & ~0x03; // Round up to nearest integer multiple

    // Load entire DLDI
    free(device);
    if ((device = malloc(dldiSize)) == NULL)
    {
        close(fd);
        return NULL;
    }

    memset(device, 0, dldiSize);
    lseek(fd, 0, SEEK_SET);
    read(fd, device, dldiSize);
    close(fd);

    dldiFixDriverAddresses(device);

    if (device->ioInterface.features & FEATURE_SLOT_GBA)
        sysSetCartOwner(BUS_OWNER_ARM9);

    if (device->ioInterface.features & FEATURE_SLOT_NDS)
        sysSetCardOwner(BUS_OWNER_ARM9);

    return device;
}

void dldiFree(DLDI_INTERFACE *dldi)
{
    free(dldi);
}
