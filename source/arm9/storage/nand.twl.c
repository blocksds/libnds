#include <nds/disc_io.h>
#include <nds/fifocommon.h>
#include <nds/fifomessages.h>
#include <nds/system.h>
#include <nds/arm9/cache.h>

#include <nds/arm9/nand.h>

//---------------------------------------------------------------------------------
bool nand_Startup() {
//---------------------------------------------------------------------------------
	fifoMutexAcquire(FIFO_STORAGE);

	fifoSendValue32(FIFO_STORAGE, SDMMC_HAVE_SD);
	fifoWaitValueAsync32(FIFO_STORAGE);
	int result = fifoGetValue32(FIFO_STORAGE);

	fifoMutexRelease(FIFO_STORAGE);

	if (result == 0)
		return false;

	fifoMutexAcquire(FIFO_STORAGE);

	fifoSendValue32(FIFO_STORAGE, SDMMC_NAND_START);
	fifoWaitValueAsync32(FIFO_STORAGE);
	result = fifoGetValue32(FIFO_STORAGE);

	fifoMutexRelease(FIFO_STORAGE);

	return result == 0;
}

//---------------------------------------------------------------------------------
bool nand_IsInserted() {
//---------------------------------------------------------------------------------
	return true;
}

//---------------------------------------------------------------------------------
bool nand_ReadSectors(sec_t sector, sec_t numSectors,void* buffer) {
//---------------------------------------------------------------------------------
	FifoMessage msg;

	DC_FlushRange(buffer, numSectors * 512);

	msg.type = SDMMC_NAND_READ_SECTORS;
	msg.sdParams.startsector = sector;
	msg.sdParams.numsectors = numSectors;
	msg.sdParams.buffer = buffer;

	fifoMutexAcquire(FIFO_STORAGE);

	fifoSendDatamsg(FIFO_STORAGE, sizeof(msg), (u8*)&msg);

	fifoWaitValueAsync32(FIFO_STORAGE);
	DC_InvalidateRange(buffer, numSectors * 512);

	int result = fifoGetValue32(FIFO_STORAGE);

	fifoMutexRelease(FIFO_STORAGE);

	return result == 0;
}

//---------------------------------------------------------------------------------
bool nand_WriteSectors(sec_t sector, sec_t numSectors,const void* buffer) {
//---------------------------------------------------------------------------------
	FifoMessage msg;

	DC_FlushRange(buffer, numSectors * 512);

	msg.type = SDMMC_NAND_WRITE_SECTORS;
	msg.sdParams.startsector = sector;
	msg.sdParams.numsectors = numSectors;
	msg.sdParams.buffer = (void*)buffer;

	fifoMutexAcquire(FIFO_STORAGE);

	fifoSendDatamsg(FIFO_STORAGE, sizeof(msg), (u8*)&msg);

	fifoWaitValueAsync32(FIFO_STORAGE);
	DC_InvalidateRange(buffer, numSectors * 512);

	int result = fifoGetValue32(FIFO_STORAGE);

	fifoMutexRelease(FIFO_STORAGE);

	return result == 0;
}


//---------------------------------------------------------------------------------
bool nand_ClearStatus() {
//---------------------------------------------------------------------------------
	return true;
}

//---------------------------------------------------------------------------------
bool nand_Shutdown() {
//---------------------------------------------------------------------------------
	return true;
}

//---------------------------------------------------------------------------------
ssize_t nand_GetSize() {
//---------------------------------------------------------------------------------
	fifoMutexAcquire(FIFO_STORAGE);

	fifoSendValue32(FIFO_STORAGE, SDMMC_NAND_SIZE);
	fifoWaitValueAsync32(FIFO_STORAGE);
	ssize_t result = fifoGetValue32(FIFO_STORAGE);

	fifoMutexRelease(FIFO_STORAGE);

	return result;

}
/*const DISC_INTERFACE __io_dsisd = {
	DEVICE_TYPE_DSI_SD,
	FEATURE_MEDIUM_CANREAD | FEATURE_MEDIUM_CANWRITE,
	(FN_MEDIUM_STARTUP)&sdio_Startup,
	(FN_MEDIUM_ISINSERTED)&sdio_IsInserted,
	(FN_MEDIUM_READSECTORS)&sdio_ReadSectors,
	(FN_MEDIUM_WRITESECTORS)&sdio_WriteSectors,
	(FN_MEDIUM_CLEARSTATUS)&sdio_ClearStatus,
	(FN_MEDIUM_SHUTDOWN)&sdio_Shutdown
};
*/
