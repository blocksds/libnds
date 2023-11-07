// SPDX-License-Identifier: Zlib
//
// Copyright (c) 2023 Antonio Niño Díaz

#include <stddef.h>

#include <nds/card.h>
#include <nds/disc_io.h>
#include <nds/fifocommon.h>
#include <nds/fifomessages.h>
#include <nds/interrupts.h>
#include <nds/system.h>

static const DISC_INTERFACE *dldi_io = NULL;

int sdmmcMsgHandler(int bytes, void *user_data, FifoMessage *msg);
int sdmmcValueHandler(u32 value, void *user_data);

void storageMsgHandler(int bytes, void *user_data)
{
    FifoMessage msg;
    int retval = 0;

    fifoGetDatamsg(FIFO_STORAGE, bytes, (u8*)&msg);

    int oldIME = enterCriticalSection();
    switch (msg.type)
    {
        case SDMMC_SD_READ_SECTORS:
        case SDMMC_SD_WRITE_SECTORS:
        case SDMMC_NAND_READ_SECTORS:
        case SDMMC_NAND_WRITE_SECTORS:
            if (isDSiMode())
                retval = sdmmcMsgHandler(bytes, user_data, &msg);
            break;

        case DLDI_STARTUP:
            dldi_io = msg.dldiStartupParams.io_interface;
            if (dldi_io)
                retval = dldi_io->startup();
            break;

        case DLDI_READ_SECTORS:
            if (dldi_io)
            {
                retval = dldi_io->readSectors(msg.sdParams.startsector,
                                            msg.sdParams.numsectors,
                                            msg.sdParams.buffer);
            }
            break;

        case DLDI_WRITE_SECTORS:
            if (dldi_io)
            {
                retval = dldi_io->writeSectors(msg.sdParams.startsector,
                                            msg.sdParams.numsectors,
                                            msg.sdParams.buffer);
            }
            break;
    }

    leaveCriticalSection(oldIME);

    fifoSendValue32(FIFO_STORAGE, retval);
}

void storageValueHandler(u32 value, void *user_data)
{
    int result = 0;
    int oldIME = enterCriticalSection();

    switch (value)
    {
        case SDMMC_HAVE_SD:
        case SDMMC_SD_START:
        case SDMMC_NAND_START:
        case SDMMC_SD_IS_INSERTED:
        case SDMMC_SD_STOP:
        case SDMMC_NAND_SIZE:
            if (isDSiMode())
                result = sdmmcValueHandler(value, user_data);
            break;

        case DLDI_IS_INSERTED:
            if (dldi_io)
                result = dldi_io->isInserted();
            break;

        case DLDI_CLEAR_STATUS:
            if (dldi_io)
                result = dldi_io->clearStatus();
            break;

        case DLDI_SHUTDOWN:
            if (dldi_io)
                result = dldi_io->shutdown();
            break;
    }

    leaveCriticalSection(oldIME);

    fifoSendValue32(FIFO_STORAGE, result);
}
