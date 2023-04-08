// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2008 Jason Rogers (Dovoto)

// Sound Functions

#ifndef FIFOMESSAGE_H
#define FIFOMESSAGE_H

#include "ndstypes.h"
#include <nds/touch.h>

/* internal fifo messages utilized by libnds. */

typedef enum {
	SOUND_PLAY_MESSAGE = 0x1234,
	SOUND_PSG_MESSAGE,
	SOUND_NOISE_MESSAGE,
	MIC_RECORD_MESSAGE,
	MIC_BUFFER_FULL_MESSAGE,
	SYS_INPUT_MESSAGE,
	SDMMC_SD_READ_SECTORS,
	SDMMC_SD_WRITE_SECTORS,
	SDMMC_NAND_READ_SECTORS,
	SDMMC_NAND_WRITE_SECTORS,
	CAMERA_APT_READ_I2C,
	CAMERA_APT_WRITE_I2C,
	CAMERA_APT_READ_MCU,
	CAMERA_APT_WRITE_MCU
} FifoMessageType;

typedef struct FifoMessage {
	u16 type;

	union {

		struct {
			const void* data;
			u32 dataSize;
			u16 loopPoint;
			u16 freq;
			u8 volume;
			u8 pan;
			bool loop;
			u8 format;
			u8 channel;
		} SoundPlay;

		struct {
			u16 freq;
			u8 dutyCycle;
			u8 volume;
			u8 pan;
			u8 channel;
		} SoundPsg;

		struct {
			void* buffer;
			u32 bufferLength;
			u16 freq;
			u8 format;
		} MicRecord;

		struct {
			void* buffer;
			u32 length;
		} MicBufferFull;

		struct {
			touchPosition touch;
			u16 keys;
		} SystemInput;

		struct {
			void *io_interface;
		} dldiStartupParams;

		struct {
			void *buffer;
			u32 startsector;
			u32 numsectors;
		} sdParams;

		struct {
			void *buffer;
			u32 offset;
			u32 size;
		} cardParams;

		struct {
			void *buffer;
			u32 address;
			u32 length;
		} blockParams;

		struct {
			u16 reg;
			u16 value;
			u8 device;
		} aptRegParams;
	};

} ALIGN(4) FifoMessage;


#endif
