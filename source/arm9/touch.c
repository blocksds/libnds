// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2005-2008 Dave Murphy (WinterMute)

// Touch screen input code

#include <nds/ipc.h>
#include <nds/arm9/input.h>
#include <nds/fifocommon.h>
#include <libnds_internal.h>

void touchRead(touchPosition *data)
{
	if ( !data ) return;

	data->rawx = __transferRegion()->touchX;
	data->rawy = __transferRegion()->touchY;
	data->px = __transferRegion()->touchXpx;
	data->py = __transferRegion()->touchYpx;
	data->z1 = __transferRegion()->touchZ1;
	data->z2 = __transferRegion()->touchZ2;


}

touchPosition touchReadXY()
{
	touchPosition touchPos;

	touchRead(&touchPos);
	return touchPos;
}
