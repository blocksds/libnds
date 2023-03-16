/*
 disc.c
 Interface to the low level disc functions. Used by the higher level
 file system code.
 Based on code originally written by MightyMax

 Copyright (c) 2006 Michael "Chishm" Chisholm
	
 Redistribution and use in source and binary forms, with or without modification,
 are permitted provided that the following conditions are met:

  1. Redistributions of source code must retain the above copyright notice,
     this list of conditions and the following disclaimer.
  2. Redistributions in binary form must reproduce the above copyright notice,
     this list of conditions and the following disclaimer in the documentation and/or
     other materials provided with the distribution.
  3. The name of the author may not be used to endorse or promote products derived
     from this software without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE
 LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <nds/arm9/cache.h>
#include <nds/arm9/dldi.h>
#include <nds/fifocommon.h>
#include <nds/fifomessages.h>
#include <nds/memory.h>
#include <nds/system.h>

#include <string.h>
#include <malloc.h>
#include <unistd.h>
#include <sys/fcntl.h>

const u32  DLDI_MAGIC_NUMBER = 
	0xBF8DA5ED;	
	
// Stored backwards to prevent it being picked up by DLDI patchers
const char DLDI_MAGIC_STRING_BACKWARDS [DLDI_MAGIC_STRING_LEN] =
	{'\0', 'm', 'h', 's', 'i', 'h', 'C', ' '} ;

// The only built in driver
extern DLDI_INTERFACE _io_dldi_stub;

const DLDI_INTERFACE* io_dldi_data = &_io_dldi_stub;

// -----------------------------------------------------------------------------

bool dldi_arm7_startup(void)
{
	FifoMessage msg;
	msg.type = DLDI_STARTUP;
	msg.sdParams.buffer = &_io_dldi_stub.ioInterface;

	fifoSendDatamsg(FIFO_SDMMC, sizeof(msg), (u8 *)&msg);

	fifoWaitValue32(FIFO_SDMMC);

	int result = fifoGetValue32(FIFO_SDMMC);
	return result != 0;
}

bool dldi_arm7_is_inserted(void)
{
	fifoSendValue32(FIFO_SDMMC, DLDI_IS_INSERTED);

	fifoWaitValue32(FIFO_SDMMC);

	int result = fifoGetValue32(FIFO_SDMMC);
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

	fifoSendDatamsg(FIFO_SDMMC, sizeof(msg), (u8 *)&msg);

	fifoWaitValue32(FIFO_SDMMC);

	int result = fifoGetValue32(FIFO_SDMMC);
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

	fifoSendDatamsg(FIFO_SDMMC, sizeof(msg), (u8 *)&msg);

	fifoWaitValue32(FIFO_SDMMC);

	int result = fifoGetValue32(FIFO_SDMMC);
	return result != 0;
}

bool dldi_arm7_clear_status(void)
{
	fifoSendValue32(FIFO_SDMMC, DLDI_CLEAR_STATUS);

	fifoWaitValue32(FIFO_SDMMC);

	int result = fifoGetValue32(FIFO_SDMMC);
	return result != 0;
}

bool dldi_arm7_shutdown(void)
{
	fifoSendValue32(FIFO_SDMMC, DLDI_SHUTDOWN);

	fifoWaitValue32(FIFO_SDMMC);

	int result = fifoGetValue32(FIFO_SDMMC);
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

const DISC_INTERFACE* dldiGetInternal(void)
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


bool dldiIsValid (const DLDI_INTERFACE* io) {
	int i;
	
	if (io->magicNumber != DLDI_MAGIC_NUMBER) {
		return false;
	}
	
	for (i = 0; i < DLDI_MAGIC_STRING_LEN; i++) {
		if (io->magicString[i] != DLDI_MAGIC_STRING_BACKWARDS [DLDI_MAGIC_STRING_LEN - 1 - i]) {
			return false;
		}
	}
	
	return true;
}

void dldiFixDriverAddresses (DLDI_INTERFACE* io) {
	u32 offset;
	u8** address;
	u8* oldStart;
	u8* oldEnd;
	
	offset = (char*)io - (char*)(io->dldiStart);

	oldStart = io->dldiStart;
	oldEnd = io->dldiEnd;
	
	// Correct all pointers to the offsets from the location of this interface
	io->dldiStart 		= (char*)io->dldiStart + offset;
	io->dldiEnd 		= (char*)io->dldiEnd + offset;
	io->interworkStart 	= (char*)io->interworkStart + offset;
	io->interworkEnd 	= (char*)io->interworkEnd + offset;
	io->gotStart 		= (char*)io->gotStart + offset;
	io->gotEnd 			= (char*)io->gotEnd + offset;
	io->bssStart 		= (char*)io->bssStart + offset;
	io->bssEnd 			= (char*)io->bssEnd + offset;
	
	io->ioInterface.startup 		= (FN_MEDIUM_STARTUP)		((intptr_t)io->ioInterface.startup + offset);
	io->ioInterface.isInserted 		= (FN_MEDIUM_ISINSERTED)	((intptr_t)io->ioInterface.isInserted + offset);
	io->ioInterface.readSectors 	= (FN_MEDIUM_READSECTORS)	((intptr_t)io->ioInterface.readSectors + offset);
	io->ioInterface.writeSectors	= (FN_MEDIUM_WRITESECTORS)	((intptr_t)io->ioInterface.writeSectors + offset);
	io->ioInterface.clearStatus 	= (FN_MEDIUM_CLEARSTATUS)	((intptr_t)io->ioInterface.clearStatus + offset);
	io->ioInterface.shutdown 		= (FN_MEDIUM_SHUTDOWN)		((intptr_t)io->ioInterface.shutdown + offset);

	// Fix all addresses with in the DLDI
	if (io->fixSectionsFlags & FIX_ALL) {
		for (address = (u8**)io->dldiStart; address < (u8**)io->dldiEnd; address++) {
			if (oldStart <= *address && *address < oldEnd) {
				*address += offset;
			}
		}
	}
	
	// Fix the interworking glue section
	if (io->fixSectionsFlags & FIX_GLUE) {
		for (address = (u8**)io->interworkStart; address < (u8**)io->interworkEnd; address++) {
			if (oldStart <= *address && *address < oldEnd) {
				*address += offset;
			}
		}
	}
	
	// Fix the global offset table section
	if (io->fixSectionsFlags & FIX_GOT) {
		for (address = (u8**)io->gotStart; address < (u8**)io->gotEnd; address++) {
			if (oldStart <= *address && *address < oldEnd) {
				*address += offset;
			}
		}
	}
	
	// Initialise the BSS to 0
	if (io->fixSectionsFlags & FIX_BSS) {
		memset (io->bssStart, 0, (u8*)io->bssEnd - (u8*)io->bssStart);
	}
}

DLDI_INTERFACE* dldiLoadFromFile (const char* path) {
	DLDI_INTERFACE* device;
	int fd;
	size_t dldiSize;

	// Read in the DLDI header
	if ((fd = open (path, O_RDONLY, 0)) < 0) {
		return NULL;
	}
	
	if ((device = malloc (sizeof(DLDI_INTERFACE))) == NULL) {
		close (fd);
		return NULL;
	}
	
	if (read (fd, device, sizeof(DLDI_INTERFACE)) < sizeof(DLDI_INTERFACE)) {
		free (device);
		close (fd);
		return NULL;
	}
	
	// Check that it is a valid DLDI
	if (!dldiIsValid (device)) {
		free (device);
		close (fd);
		return NULL;
	}
	
	// Calculate actual size of DLDI
	// Although the file may only go to the dldiEnd, the BSS section can extend past that
	if (device->dldiEnd > device->bssEnd) {
		dldiSize = (char*)device->dldiEnd - (char*)device->dldiStart;
	} else {
		dldiSize = (char*)device->bssEnd - (char*)device->dldiStart;
	}
	dldiSize = (dldiSize + 0x03) & ~0x03; 		// Round up to nearest integer multiple

	// Load entire DLDI
	free (device);
	if ((device = malloc (dldiSize)) == NULL) {
		close (fd);
		return NULL;
	}
	
	memset (device, 0, dldiSize);
	lseek (fd, 0, SEEK_SET);
	read (fd, device, dldiSize);
	close (fd);
	
	dldiFixDriverAddresses (device);

	if (device->ioInterface.features & FEATURE_SLOT_GBA) {
		sysSetCartOwner(BUS_OWNER_ARM9);
	}
	if (device->ioInterface.features & FEATURE_SLOT_NDS) {
		sysSetCardOwner(BUS_OWNER_ARM9);
	}
	
	return device;
}

void dldiFree (DLDI_INTERFACE* dldi) {
	free(dldi);
}
