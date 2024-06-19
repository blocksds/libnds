// SPDX-License-Identifier: Zlib
//
// Copyright (c) 2023 Adrian "asie" Siekierka

// Camera control for the ARM7

#include <nds/arm7/camera.h>
#include <nds/arm7/i2c.h>
#include <nds/camera.h>
#include <nds/fifocommon.h>
#include <nds/fifomessages.h>
#include <nds/ipc.h>
#include <nds/system.h>

// Low-level Aptina I2C register read/write functions

void aptI2cWaitClearBits(u8 device, u16 reg, u16 mask)
{
    while (aptI2cRead(device, reg) & mask)
        ;
}

void aptI2cWaitSetBits(u8 device, u16 reg, u16 mask)
{
    while ((aptI2cRead(device, reg) & mask) != mask)
        ;
}

void aptI2cClearBits(u8 device, u16 reg, u16 mask)
{
    u16 temp = aptI2cRead(device, reg);
    aptI2cWrite(device, reg, temp & ~mask);
}

void aptI2cSetBits(u8 device, u16 reg, u16 mask)
{
    u16 temp = aptI2cRead(device, reg);
    aptI2cWrite(device, reg, temp | mask);
}

// Low-level Aptina MCU register read/write functions

u16 aptMcuRead(u8 device, u16 reg)
{
    aptI2cWrite(device, I2CREG_APT_MCU_ADDRESS, reg);
    return aptI2cRead(device, I2CREG_APT_MCU_DATA0);
}

void aptMcuWrite(u8 device, u16 reg, u16 data)
{
    aptI2cWrite(device, I2CREG_APT_MCU_ADDRESS, reg);
    aptI2cWrite(device, I2CREG_APT_MCU_DATA0, data);
}

void aptMcuWaitClearBits(u8 device, u16 reg, u16 mask)
{
    while (aptMcuRead(device, reg) & mask)
        ;
}

void aptMcuWaitSetBits(u8 device, u16 reg, u16 mask)
{
    while ((aptMcuRead(device, reg) & mask) != mask)
        ;
}

void aptMcuClearBits(u8 device, u16 reg, u16 mask)
{
    u16 temp = aptMcuRead(device, reg);
    aptMcuWrite(device, reg, temp & ~mask);
}

void aptMcuSetBits(u8 device, u16 reg, u16 mask)
{
    u16 temp = aptMcuRead(device, reg);
    aptMcuWrite(device, reg, temp | mask);
}

// High-level camera functions

void aptCameraSeqCmd(u8 device, u8 cmd)
{
    aptMcuWrite(device, MCUREG_APT_SEQ_CMD, cmd);
    aptMcuWaitClearBits(device, MCUREG_APT_SEQ_CMD, MCUREG_APT_SEQ_CMD_MASK);
}

static void aptCameraWakeup(u8 device)
{
    aptI2cClearBits(device, I2CREG_APT_STANDBY_CNT, I2CREG_APT_STANDBY_ENABLE);
    aptI2cWaitClearBits(device, I2CREG_APT_STANDBY_CNT, I2CREG_APT_STANDBY_STATUS);
    aptI2cWaitSetBits(device, 0x301A, 0x0004);
}

static void aptCameraStandby(u8 device)
{
    aptI2cSetBits(device, I2CREG_APT_STANDBY_CNT, I2CREG_APT_STANDBY_ENABLE);
    aptI2cWaitSetBits(device, I2CREG_APT_STANDBY_CNT, I2CREG_APT_STANDBY_STATUS);
    aptI2cWaitClearBits(device, 0x301A, 0x0004);
}

static void i2cCameraLedSet(u8 value)
{
    i2cWriteRegister(I2C_PM, I2CREGPM_CAMLED, value);
}

void aptCameraInit(u8 device)
{
    // Reset
    aptI2cWrite(device, I2CREG_APT_RESET_MISC_CNT,
                I2CREG_APT_MIPI_TX_RESET | I2CREG_APT_I2C_RESET);
    aptI2cWrite(device, I2CREG_APT_RESET_MISC_CNT, 0);

    // Initial wakeup
    aptI2cWrite(device, I2CREG_APT_STANDBY_CNT,
                I2CREG_APT_STANDBY_STATUS | I2CREG_APT_STANDBY_IRQ_ENABLE | (1 << 5));
    aptI2cWrite(device, I2CREG_APT_PAD_SLEW,
                I2CREG_APT_PARALLEL_OUT_SLEW_RATE(1) | I2CREG_APT_PCLK_SLEW_RATE(2));
    aptI2cWrite(device, I2CREG_APT_CLOCKS_CNT, I2CREG_APT_CLKIN_ENABLE | 0x40DF);

    // Wait for wakeup
    aptI2cWaitClearBits(device, I2CREG_APT_STANDBY_CNT, I2CREG_APT_STANDBY_STATUS);
    aptI2cWaitSetBits(device, 0x301A, 0x0004);

    aptMcuWrite(device, 0x02F0, 0x0000);
    aptMcuWrite(device, 0x02F2, 0x0210);
    aptMcuWrite(device, 0x02F4, 0x001A);
    aptMcuWrite(device, 0x2145, 0x02F4);
    aptMcuWrite(device, MCUREG_APT_8BIT | 0x2134, 0x01);

    aptMcuSetBits(device, MCUREG_APT_SEQ_CAP_MODE, MCUREG_APT_SEQ_CAP_MODE_VIDEO_ENABLE);
    aptMcuWrite(device, MCUREG_APT_MODE_A_OUTPUT_FORMAT,
                MCUREG_APT_MODE_OUTPUT_FORMAT_YUV
                | MCUREG_APT_MODE_OUTPUT_FORMAT_SWAP_LUMA_CHROMA);
    aptMcuWrite(device, MCUREG_APT_MODE_B_OUTPUT_FORMAT,
                MCUREG_APT_MODE_OUTPUT_FORMAT_YUV
                | MCUREG_APT_MODE_OUTPUT_FORMAT_SWAP_LUMA_CHROMA);

    // Match PLL to console timings
    aptI2cWrite(device, I2CREG_APT_PLL_CNT,
                0x2044 | I2CREG_APT_PLL_RESET_CNTR | I2CREG_APT_PLL_BYPASS);
    aptI2cWrite(device, I2CREG_APT_PLL_DIVS, I2CREG_APT_PLL_M(17) | I2CREG_APT_PLL_N(1));
    aptI2cWrite(device, I2CREG_APT_PLL_P_DIVS,
                I2CREG_APT_PLL_P1(0) | I2CREG_APT_PLL_P3(0));
    aptI2cWrite(device, I2CREG_APT_PLL_CNT,
                0x2448 | I2CREG_APT_PLL_ENABLE | I2CREG_APT_PLL_BYPASS);
    aptI2cWrite(device, I2CREG_APT_PLL_CNT,
                0x3048 | I2CREG_APT_PLL_ENABLE | I2CREG_APT_PLL_BYPASS);
    aptI2cWaitSetBits(device, I2CREG_APT_PLL_CNT, I2CREG_APT_PLL_LOCK);
    aptI2cClearBits(device, I2CREG_APT_PLL_CNT, I2CREG_APT_PLL_BYPASS);

    // Configure output sizes
    aptMcuWrite(device, MCUREG_APT_MODE_A_OUTPUT_WIDTH, 256);
    aptMcuWrite(device, MCUREG_APT_MODE_A_OUTPUT_HEIGHT, 192);
    aptMcuWrite(device, MCUREG_APT_MODE_B_OUTPUT_WIDTH, 640);
    aptMcuWrite(device, MCUREG_APT_MODE_B_OUTPUT_HEIGHT, 480);

    // Configure sensors
    u16 sensorReadMode = MCUREG_APT_READ_X_ODD_INC(1) | MCUREG_APT_READ_Y_ODD_INC(1);
    if (device == I2C_CAM1)
        sensorReadMode |= MCUREG_APT_READ_X_MIRROR;

    aptMcuWrite(device, MCUREG_APT_MODE_A_SENSOR_ROW_SPEED, 1);
    aptMcuWrite(device, MCUREG_APT_MODE_A_SENSOR_FINE_CORRECTION, 26);
    aptMcuWrite(device, MCUREG_APT_MODE_A_SENSOR_FINE_IT_MIN, 107);
    aptMcuWrite(device, MCUREG_APT_MODE_A_SENSOR_FINE_IT_MAX_MARGIN, 107);
    aptMcuWrite(device, MCUREG_APT_MODE_A_SENSOR_FRAME_LENGTH, 704);
    aptMcuWrite(device, MCUREG_APT_MODE_A_SENSOR_LINE_LENGTH_PCK, 843);
    aptMcuWrite(device, MCUREG_APT_AE_MIN_INDEX, 0);
    aptMcuWrite(device, MCUREG_APT_AE_MAX_INDEX, 6);
    aptMcuWrite(device, MCUREG_APT_MODE_B_SENSOR_ROW_SPEED, 1);
    aptMcuWrite(device, MCUREG_APT_MODE_B_SENSOR_FINE_CORRECTION, 26);
    aptMcuWrite(device, MCUREG_APT_MODE_B_SENSOR_FINE_IT_MIN, 107);
    aptMcuWrite(device, MCUREG_APT_MODE_B_SENSOR_FINE_IT_MAX_MARGIN, 107);
    aptMcuWrite(device, MCUREG_APT_MODE_B_SENSOR_FRAME_LENGTH, 704);
    aptMcuWrite(device, MCUREG_APT_MODE_B_SENSOR_LINE_LENGTH_PCK, 843);
    aptI2cSetBits(device, I2CREG_APT_COLOR_PIPELINE_CNT,
                  I2CREG_APT_PGA_PIXEL_SHADING_CORRECT_ENABLE);
    aptMcuWrite(device, MCUREG_APT_8BIT | 0x2208, 0x00);
    aptMcuWrite(device, MCUREG_APT_AE_TARGET_BUFFER_SPEED, 32);
    aptMcuWrite(device, MCUREG_APT_AE_TARGET_BASE, 112);
    aptMcuWrite(device, MCUREG_APT_MODE_A_SENSOR_READ_MODE, sensorReadMode);
    aptMcuWrite(device, MCUREG_APT_MODE_B_SENSOR_READ_MODE, sensorReadMode);
    if (device == I2C_CAM0)
    {
        aptMcuWrite(device, MCUREG_APT_AE_WINDOW_POS,
                    MCUREG_APT_AE_WINDOW_X0(2) | MCUREG_APT_AE_WINDOW_Y0(2));
        aptMcuWrite(device, MCUREG_APT_AE_WINDOW_SIZE,
                    MCUREG_APT_AE_WINDOW_WIDTH(11) | MCUREG_APT_AE_WINDOW_HEIGHT(11));
    }
    else
    {
        aptMcuWrite(device, MCUREG_APT_AE_WINDOW_POS,
                    MCUREG_APT_AE_WINDOW_X0(0) | MCUREG_APT_AE_WINDOW_Y0(0));
        aptMcuWrite(device, MCUREG_APT_AE_WINDOW_SIZE,
                    MCUREG_APT_AE_WINDOW_WIDTH(15) | MCUREG_APT_AE_WINDOW_HEIGHT(15));
    }
    aptI2cSetBits(device, I2CREG_APT_CLOCKS_CNT, 1 << 5);
    aptMcuWrite(device, MCUREG_APT_SEQ_CAP_MODE,
                0x40 | MCUREG_APT_SEQ_CAP_MODE_VIDEO_HG_ENABLE
                | MCUREG_APT_SEQ_CAP_MODE_VIDEO_AWB_ENABLE
                | MCUREG_APT_SEQ_CAP_MODE_VIDEO_ENABLE);
    aptMcuWrite(device, MCUREG_APT_SEQ_PREVIEW1_AWB, 0x01);
    if (device == I2C_CAM0)
    {
        aptI2cWrite(device, I2CREG_APT_APERTURE_PARAMS,
                    I2CREG_APT_APERTURE_GAIN(1) | I2CREG_APT_APERTURE_GAIN_EXP(1));
        aptMcuWrite(device, MCUREG_APT_HG_LL_AP_CORR1, 1);
    }
    else
    {
        aptI2cWrite(device, I2CREG_APT_APERTURE_PARAMS,
                    I2CREG_APT_APERTURE_GAIN(0) | I2CREG_APT_APERTURE_GAIN_EXP(2));
        aptMcuWrite(device, MCUREG_APT_HG_LL_AP_CORR1, 2);
    }

    aptCameraSeqCmd(device, MCUREG_APT_SEQ_CMD_REFRESH_MODE);
    aptCameraSeqCmd(device, MCUREG_APT_SEQ_CMD_REFRESH);
}

void aptCameraDeinit(u8 device)
{
    (void)device;
    // TODO: Do we need to do anything here?
}

void aptCameraActivate(u8 device)
{
    if (device == 0xFF)
        return;

    aptCameraWakeup(device);
    aptI2cSetBits(device, I2CREG_APT_RESET_MISC_CNT, I2CREG_APT_PARALLEL_ENABLE);
    if (device == I2C_CAM1)
        i2cCameraLedSet(1);
}

void aptCameraDeactivate(u8 device)
{
    if (device == 0xFF)
        return;

    aptI2cClearBits(device, I2CREG_APT_RESET_MISC_CNT, I2CREG_APT_PARALLEL_ENABLE);
    aptCameraStandby(device);
    if (device == I2C_CAM1)
        i2cCameraLedSet(0);
}

void cameraDataHandler(int bytes, void *userData)
{
    (void)userData;

    FifoMessage msg;
    fifoGetDatamsg(FIFO_CAMERA, bytes, (u8 *)&msg);

    u8 device = I2C_CAM0;
    if (msg.aptRegParams.device > 1)
    {
        fifoSendValue32(FIFO_CAMERA, 0);
        return;
    }

    if (msg.aptRegParams.device == 1)
        device = I2C_CAM1;

    switch (msg.type)
    {
        case CAMERA_APT_READ_I2C:
            fifoSendValue32(FIFO_CAMERA, aptI2cRead(device, msg.aptRegParams.reg));
            break;
        case CAMERA_APT_WRITE_I2C:
            fifoSendValue32(FIFO_CAMERA, aptI2cWrite(device, msg.aptRegParams.reg,
                                                     msg.aptRegParams.value));
            break;
        case CAMERA_APT_READ_MCU:
            fifoSendValue32(FIFO_CAMERA, aptMcuRead(device, msg.aptRegParams.reg));
            break;
        case CAMERA_APT_WRITE_MCU:
            aptMcuWrite(device, msg.aptRegParams.reg, msg.aptRegParams.value);
            fifoSendValue32(FIFO_CAMERA, 1);
            break;
    }
}

// Camera FIFO handler

static u8 activeDevice = 0xFF;

static u8 getDeviceFromFifoValue(u32 fifoValue)
{
    fifoValue &= 0xFF;
    return fifoValue > 1 ? 0xFF : (fifoValue == 1 ? I2C_CAM1 : I2C_CAM0);
}

void cameraCommandHandler(u32 fifoValue, void *userdata)
{
    (void)userdata;

    u32 command = (fifoValue >> 22);

    switch (command)
    {
        case CAMERA_CMD_INIT:
            aptCameraInit(I2C_CAM0);
            aptCameraInit(I2C_CAM1);
            // TODO: The 3DS doesn't need to deactivate the camera for it to
            // work. On DSi, the image appears flipped on the Y axis and with
            // color components swapped. Why? Is there a smaller set of commands
            // that make it work reliably?
            aptCameraDeactivate(I2C_CAM0);
            aptCameraDeactivate(I2C_CAM1);
            fifoSendValue32(FIFO_CAMERA,
                            aptI2cRead(I2C_CAM0, I2CREG_APT_CHIP_VERSION));
            break;

        case CAMERA_CMD_DEINIT:
            aptCameraDeactivate(activeDevice);
            activeDevice = 0xFF;
            aptCameraDeinit(I2C_CAM1);
            aptCameraDeinit(I2C_CAM0);
            fifoSendValue32(FIFO_CAMERA, 1);
            break;

        case CAMERA_CMD_SELECT:
            aptCameraDeactivate(activeDevice);
            activeDevice = getDeviceFromFifoValue(fifoValue);
            aptCameraActivate(activeDevice);
            fifoSendValue32(FIFO_CAMERA, 1);
            break;

        case CAMERA_CMD_SEND_SEQ_CMD:
            aptCameraSeqCmd(activeDevice, fifoValue & MCUREG_APT_SEQ_CMD_MASK);
            fifoSendValue32(FIFO_CAMERA, 1);
            break;
    }
}

void installCameraFIFO(void)
{
    fifoSetDatamsgHandler(FIFO_CAMERA, cameraDataHandler, 0);
    fifoSetValue32Handler(FIFO_CAMERA, cameraCommandHandler, 0);
}
