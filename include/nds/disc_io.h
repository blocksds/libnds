// SPDX-License-Identifier: BSD-3-Clause
//
// Copyright (c) 2006 Michael "Chishm" Chisholm

// Interface template for low level disc functions. Based on code originally
// written by MightyMax.

#ifndef NDS_DISC_IO_INCLUDE
#define NDS_DISC_IO_INCLUDE

#include "ndstypes.h"

#define FEATURE_MEDIUM_CANREAD		0x00000001
#define FEATURE_MEDIUM_CANWRITE		0x00000002
#define FEATURE_SLOT_GBA			0x00000010
#define FEATURE_SLOT_NDS			0x00000020

#define DEVICE_TYPE_DSI_SD ('_') | ('S' << 8) | ('D' << 16) | ('_' << 24)

typedef bool (* FN_MEDIUM_STARTUP)(void) ;
typedef bool (* FN_MEDIUM_ISINSERTED)(void) ;
typedef bool (* FN_MEDIUM_READSECTORS)(sec_t sector, sec_t numSectors, void* buffer) ;
typedef bool (* FN_MEDIUM_WRITESECTORS)(sec_t sector, sec_t numSectors, const void* buffer) ;
typedef bool (* FN_MEDIUM_CLEARSTATUS)(void) ;
typedef bool (* FN_MEDIUM_SHUTDOWN)(void) ;

struct DISC_INTERFACE_STRUCT {
	unsigned long			ioType ;
	unsigned long			features ;
	FN_MEDIUM_STARTUP		startup ;
	FN_MEDIUM_ISINSERTED	isInserted ;
	FN_MEDIUM_READSECTORS	readSectors ;
	FN_MEDIUM_WRITESECTORS	writeSectors ;
	FN_MEDIUM_CLEARSTATUS	clearStatus ;
	FN_MEDIUM_SHUTDOWN		shutdown ;
} ;

typedef struct DISC_INTERFACE_STRUCT DISC_INTERFACE ;


const DISC_INTERFACE* get_io_dsisd (void);

#endif	// define NDS_DISC_IO_INCLUDE
