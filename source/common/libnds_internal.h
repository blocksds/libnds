// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2005-2008 Dave Murphy (WinterMute)

// Internal variables for libnds

#ifndef _libnds_internal_h_
#define _libnds_internal_h_

#include <time.h>
#include <nds/ndstypes.h>
#include <nds/arm9/input.h>
#include <nds/system.h>

typedef struct __TransferRegion {
	vs16 touchX,   touchY;		// TSC X, Y
	vs16 touchXpx, touchYpx;	// TSC X, Y pixel values
	vs16 touchZ1,  touchZ2;		// TSC x-panel measurements
	vu16 buttons;				// X, Y, /PENIRQ buttons
	time_t	unixTime;
	struct __bootstub *bootcode;
} __TransferRegion, * __pTransferRegion;

#define transfer (*(__TransferRegion volatile *)(0x02FFF000))

static inline void setTransferInputData(touchPosition *touch, u16 buttons)
{
	transfer.buttons = buttons;
	transfer.touchX = touch->rawx;
	transfer.touchY = touch->rawy;
	transfer.touchXpx = touch->px;
	transfer.touchYpx = touch->py;
	transfer.touchZ1 = touch->z1;
	transfer.touchZ2 = touch->z2;
}
static inline
__TransferRegion volatile * __transferRegion(void) {
	return &transfer;
}

#endif // _libnds_internal_h_
