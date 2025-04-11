// SPDX-License-Identifier: Zlib
//
// Copyright (c) 2023 Adrian "asie" Siekierka

// Camera control for the ARM9

#include <stdio.h>

#include <nds.h>
#include <nds/fifomessages.h>

// High-level functions

extern u8 cameraActiveDevice;
struct camera_state {
    int8_t last_mode;
    uint8_t  dma_scanlines; //
    uint16_t dma_wlength; //
    uint16_t dma_blength; //for future use
    uint16_t height;//
    uint16_t width;//
} __camera_state;

//for future use
int updateDmaParam(uint16_t width, uint16_t height){
    //dma can transfer 512 words
    if (width==0 || height==0)
        return false;
    uint8_t bytes_per_pixel=2;
    uint32_t bytes_per_scanline=(uint32_t)width*bytes_per_pixel;
    uint32_t words_per_scanline=(bytes_per_scanline+4-1)/4;//ceil division
    uint32_t wordCount=(uint32_t)height*words_per_scanline;
    uint8_t scanlines=512/words_per_scanline; //floor division
    if (scanlines>16)
        return false;
    __camera_state.dma_wlength=wordCount; //total number of words per transfer
    __camera_state.dma_blength=scanlines*words_per_scanline; //total number of words per camera interrupt
    __camera_state.dma_scanlines=scanlines; //number of scanlines after which camera interrupt happens
    return true;
}


bool cameraSetCaptureMode(u8 captureMode)
{

    if (captureMode != MCUREG_APT_SEQ_CMD_PREVIEW && captureMode != MCUREG_APT_SEQ_CMD_CAPTURE)
        return false;

    fifoMutexAcquire(FIFO_CAMERA);
    fifoSendValue32(FIFO_CAMERA, CAMERA_CMD_FIFO(CAMERA_CMD_SEND_SEQ_CMD, captureMode));
    fifoWaitValue32(FIFO_CAMERA);
    fifoGetValue32(FIFO_CAMERA);
    fifoMutexRelease(FIFO_CAMERA);
    __camera_state.last_mode=captureMode;
    return true;
}

bool cameraDeinitTWL(void)
{
    if (REG_CAM_MCNT & CAM_MCNT_PWR_18V_IO)
    {
        fifoMutexAcquire(FIFO_CAMERA);
        fifoSendValue32(FIFO_CAMERA, CAMERA_CMD_FIFO(CAMERA_CMD_DEINIT, 0));
        fifoWaitValue32(FIFO_CAMERA);
        fifoGetValue32(FIFO_CAMERA);
        fifoMutexRelease(FIFO_CAMERA);
    }

    if (!(REG_CAM_MCNT || (REG_SCFG_CLK & (SCFG_CLK_CAMERA_IF | SCFG_CLK_CAMERA_EXT))))
        return false;

    REG_CAM_CNT &= ~0x8F00;
    REG_CAM_CNT |= CAM_CNT_TRANSFER_FLUSH;
    REG_SCFG_CLK &= ~SCFG_CLK_CAMERA_EXT;
    swiDelay(30);
    REG_CAM_MCNT = 0;
    REG_SCFG_CLK &= ~SCFG_CLK_CAMERA_IF;
    swiDelay(30);
    __camera_state.last_mode=-1;
    return true;
}

bool cameraInitTWL(void)
{
    if (REG_CAM_MCNT || (REG_SCFG_CLK & (SCFG_CLK_CAMERA_IF | SCFG_CLK_CAMERA_EXT)))
        cameraDeinitTWL();

    REG_SCFG_CLK |= SCFG_CLK_CAMERA_IF;
    REG_CAM_MCNT = 0;
    swiDelay(30);
    REG_SCFG_CLK |= SCFG_CLK_CAMERA_EXT;
    swiDelay(30);
    REG_CAM_MCNT |= CAM_MCNT_RESET_DISABLE | CAM_MCNT_PWR_18V_IO;
    swiDelay(8200);
    REG_SCFG_CLK &= ~SCFG_CLK_CAMERA_EXT;
    REG_CAM_CNT &= ~CAM_CNT_TRANSFER_ENABLE;
    REG_CAM_CNT |= CAM_CNT_TRANSFER_FLUSH;
    REG_CAM_CNT = (REG_CAM_CNT & ~(0x0300)) | 0x0200;
    REG_CAM_CNT |= 0x0400;
    REG_CAM_CNT |= CAM_CNT_IRQ;
    REG_SCFG_CLK |= SCFG_CLK_CAMERA_EXT;
    swiDelay(20);

    fifoMutexAcquire(FIFO_CAMERA);
    fifoSendValue32(FIFO_CAMERA, CAMERA_CMD_FIFO(CAMERA_CMD_INIT, 0));
    fifoWaitValue32Async(FIFO_CAMERA);
    u32 result = fifoGetValue32(FIFO_CAMERA);
    fifoMutexRelease(FIFO_CAMERA);

    REG_SCFG_CLK &= ~SCFG_CLK_CAMERA_EXT;
    REG_SCFG_CLK |= SCFG_CLK_CAMERA_EXT;
    swiDelay(20);
    __camera_state.last_mode=-1;
    return result == I2CREG_APT_CHIP_VERSION_MT9V113;
}

bool cameraSelectTWL(CameraDevice device)
{
    __camera_state.last_mode=-1;
    fifoMutexAcquire(FIFO_CAMERA);
    fifoSendValue32(FIFO_CAMERA, CAMERA_CMD_FIFO(CAMERA_CMD_SELECT, device));
    fifoWaitValue32(FIFO_CAMERA);
    bool result = fifoGetValue32(FIFO_CAMERA);
    fifoMutexRelease(FIFO_CAMERA);

    if (result)
    {
        cameraActiveDevice = device;
        return true;
    }
    else
    {
        return false;
    }
}


void cameraStartDMA(u16 * buffer, u8 captureMode, u8 ndmaId)
{
    bool preview= (captureMode==MCUREG_APT_SEQ_CMD_PREVIEW );
    NDMA_SRC(ndmaId) = (u32)&REG_CAM_DATA;
    NDMA_DEST(ndmaId) = (u32)buffer;
    NDMA_LENGTH(ndmaId) = (preview ? (256 * 192) : (640 * 480)) >> 1;
    NDMA_BLENGTH(ndmaId) = preview ? 512 : 320;
    NDMA_BDELAY(ndmaId) = 2;
    NDMA_CR(ndmaId) =
        NDMA_SRC_FIX | NDMA_BLOCK_SCALER(4) | NDMA_START_CAMERA | NDMA_ENABLE;
    return;
}

bool cameraStartTransferTWL(u16 *buffer, u8 captureMode, u8 ndmaId)
{
    if (cameraTransferActive())
        cameraStopTransfer();

    if (__camera_state.last_mode!=captureMode)
    {
        bool ret=cameraSetCaptureMode(captureMode);
        if(!ret)return ret;
    }
    REG_CAM_CNT &= ~0x200F;
    if (captureMode==MCUREG_APT_SEQ_CMD_PREVIEW)
        REG_CAM_CNT |= CAM_CNT_FORMAT_RGB | CAM_CNT_SCANLINES(4);
    else
        REG_CAM_CNT |= CAM_CNT_FORMAT_YUV | CAM_CNT_SCANLINES(1);

    REG_CAM_CNT |= CAM_CNT_TRANSFER_FLUSH;
    REG_CAM_CNT |= CAM_CNT_TRANSFER_ENABLE;

    cameraStartDMA(buffer,captureMode, ndmaId);

    return true;
}

// Low-level functions

static u16 cameraLLCall(int type, u8 device, u16 reg, u16 value)
{
    FifoMessage msg;
    msg.type = type;
    msg.aptRegParams.device = device;
    msg.aptRegParams.reg = reg;
    msg.aptRegParams.value = value;

    fifoMutexAcquire(FIFO_CAMERA);
    fifoSendDatamsg(FIFO_CAMERA, sizeof(msg), (u8 *)&msg);
    fifoWaitValue32(FIFO_CAMERA);
    u16 result = fifoGetValue32(FIFO_CAMERA);
    fifoMutexRelease(FIFO_CAMERA);
    return result;
}

u16 cameraI2cReadTWL(u8 device, u16 reg)
{
    return cameraLLCall(CAMERA_APT_READ_I2C, device, reg, 0);
}

u16 cameraI2cWriteTWL(u8 device, u16 reg, u16 value)
{
    return cameraLLCall(CAMERA_APT_WRITE_I2C, device, reg, value);
}

u16 cameraMcuReadTWL(u8 device, u16 reg)
{
    return cameraLLCall(CAMERA_APT_READ_MCU, device, reg, 0);
}

u16 cameraMcuWriteTWL(u8 device, u16 reg, u16 value)
{
    return cameraLLCall(CAMERA_APT_WRITE_MCU, device, reg, value);
}
