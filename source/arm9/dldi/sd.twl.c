#include <nds/disc_io.h>
#include <nds/fifocommon.h>
#include <nds/fifomessages.h>
#include <nds/memory.h>
#include <nds/system.h>
#include <nds/arm9/cache.h>

//---------------------------------------------------------------------------------
bool sdio_Startup(void) {
//---------------------------------------------------------------------------------
	fifoSendValue32(FIFO_STORAGE, SDMMC_HAVE_SD);
	while(!fifoCheckValue32(FIFO_STORAGE));
	int result = fifoGetValue32(FIFO_STORAGE);

	if(result==0) return false;

	fifoSendValue32(FIFO_STORAGE,SDMMC_SD_START);

	fifoWaitValue32(FIFO_STORAGE);

	result = fifoGetValue32(FIFO_STORAGE);

	return result == 0;
}

//---------------------------------------------------------------------------------
bool sdio_IsInserted(void) {
//---------------------------------------------------------------------------------
	fifoSendValue32(FIFO_STORAGE, SDMMC_SD_IS_INSERTED);

	fifoWaitValue32(FIFO_STORAGE);

	int result = fifoGetValue32(FIFO_STORAGE);

	return result == 1;
}

//---------------------------------------------------------------------------------
bool sdio_ReadSectors(sec_t sector, sec_t numSectors,void* buffer) {
//---------------------------------------------------------------------------------
	FifoMessage msg;

	DC_FlushRange(buffer, numSectors * 512);

	msg.type = SDMMC_SD_READ_SECTORS;
	msg.sdParams.startsector = sector;
	msg.sdParams.numsectors = numSectors;
	msg.sdParams.buffer = buffer;
	
	fifoSendDatamsg(FIFO_STORAGE, sizeof(msg), (u8*)&msg);

	fifoWaitValue32(FIFO_STORAGE);
	DC_InvalidateRange(buffer, numSectors * 512);

	int result = fifoGetValue32(FIFO_STORAGE);
	
	return result == 0;
}

//---------------------------------------------------------------------------------
bool sdio_WriteSectors(sec_t sector, sec_t numSectors,const void* buffer) {
//---------------------------------------------------------------------------------
	FifoMessage msg;

	DC_FlushRange(buffer, numSectors * 512);

	msg.type = SDMMC_SD_WRITE_SECTORS;
	msg.sdParams.startsector = sector;
	msg.sdParams.numsectors = numSectors;
	msg.sdParams.buffer = (void*)buffer;
	
	fifoSendDatamsg(FIFO_STORAGE, sizeof(msg), (u8*)&msg);

	DC_InvalidateRange(buffer, numSectors * 512);
	fifoWaitValue32(FIFO_STORAGE);

	int result = fifoGetValue32(FIFO_STORAGE);
	
	return result == 0;
}


//---------------------------------------------------------------------------------
bool sdio_ClearStatus(void) {
//---------------------------------------------------------------------------------
	return true;
}

//---------------------------------------------------------------------------------
bool sdio_Shutdown(void) {
//---------------------------------------------------------------------------------
	return true;
}

const DISC_INTERFACE __io_dsisd = {
	DEVICE_TYPE_DSI_SD,
	FEATURE_MEDIUM_CANREAD | FEATURE_MEDIUM_CANWRITE,
	&sdio_Startup,
	&sdio_IsInserted,
	&sdio_ReadSectors,
	&sdio_WriteSectors,
	&sdio_ClearStatus,
	&sdio_Shutdown
};

const DISC_INTERFACE* get_io_dsisd (void) {
	return (isDSiMode() && __NDSHeader->unitCode ) ? &__io_dsisd : NULL;
}
