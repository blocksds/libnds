// SPDX-License-Identifier: Zlib
//
// Copyright (c) 2023 Adrian "asie" Siekierka

#ifndef NDS_ARM9_CAMERA_H__
#define NDS_ARM9_CAMERA_H__

#include <stdbool.h>
#include <stddef.h>

/* ARM9 camera registers */

#define REG_CAM_MCNT	*(vu16*)0x4004200
#define REG_CAM_CNT	*(vu16*)0x4004202
#define REG_CAM_DATA	*(vu32*)0x4004204
#define REG_CAM_SOFS	*(vu32*)0x4004210
#define REG_CAM_EOFS	*(vu32*)0x4004214

#define CAM_CNT_SCANLINES(n)		((n)-1)
#define CAM_CNT_TRANSFER_ERROR		(1<<4)
#define CAM_CNT_TRANSFER_FLUSH		(1<<5)
#define CAM_CNT_IRQ			(1<<11)
#define CAM_CNT_FORMAT_YUV		(0)
#define CAM_CNT_FORMAT_RGB		(1<<13)
#define CAM_CNT_CROP			(1<<14)
#define CAM_CNT_TRANSFER_ENABLE		(1<<15)

#define CAM_OFS_X(n) ((n)&0x3FE)
#define CMA_OFS_Y(n) (((n)&0x1FF)<<16)

/* ARM9 camera FIFO defines */

#define CAMERA_CMD_FIFO(command, arg)	((command) << 22 | (arg))

typedef enum {
	CAMERA_INNER = 0,
	CAMERA_OUTER = 1,
	CAMERA_NONE = 2
} CameraDevice;

#ifdef __cplusplus
extern "C" {
#endif

/* High-level camera functions */
u8 cameraGetActive(void);
bool cameraInit(void);
bool cameraDeinit(void);
bool cameraSelect(CameraDevice device);
bool cameraStartTransfer(u16 *buffer, u8 captureMode, u8 ndmaId);

static inline void cameraStopTransfer(void) {
	REG_CAM_CNT &= ~CAM_CNT_TRANSFER_ENABLE;
}

static inline bool cameraTransferActive(void) {
	return REG_CAM_CNT >> 15;
}

/* Low-level I2C/MCU functions */
u16 cameraI2cRead(u8 device, u16 reg);
u16 cameraI2cWrite(u8 device, u16 reg, u16 value);
u16 cameraMcuRead(u8 device, u16 reg);
u16 cameraMcuWrite(u8 device, u16 reg, u16 value);

#ifdef __cplusplus
}
#endif

#endif // NDS_ARM9_CAMERA_H__
