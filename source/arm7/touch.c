// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2005 Michael Noland (joat)
// Copyright (C) 2005-2008 Jason Rogers (Dovoto)
// Copyright (C) 2005-2017 Dave Murphy (WinterMute)
// Copyright (C) 2024 Adrian "asie" Siekierka

// Touch screen control for the ARM7

#include <stdlib.h>

#include <nds/arm7/codec.h>
#include <nds/arm7/input.h>
#include <nds/arm7/touch.h>
#include <nds/arm7/tsc.h>
#include <nds/interrupts.h>
#include <nds/ndstypes.h>
#include <nds/system.h>

#include "libnds_internal.h"

static s32 xscale, yscale;
static s32 xoffset, yoffset;

#define TOUCH_CAL_SHIFT 19

void touchInit(void)
{
    xscale = ((PersonalData->calX2px - PersonalData->calX1px) << TOUCH_CAL_SHIFT)
             / ((PersonalData->calX2) - (PersonalData->calX1));
    yscale = ((PersonalData->calY2px - PersonalData->calY1px) << TOUCH_CAL_SHIFT)
             / ((PersonalData->calY2) - (PersonalData->calY1));

    xoffset = ((PersonalData->calX1 + PersonalData->calX2) * xscale
               - ((PersonalData->calX1px + PersonalData->calX2px) << TOUCH_CAL_SHIFT))
              / 2;
    yoffset = ((PersonalData->calY1 + PersonalData->calY2) * yscale
               - ((PersonalData->calY1px + PersonalData->calY2px) << TOUCH_CAL_SHIFT))
              / 2;

    if (cdcIsAvailable())
    {
        int oldIME = enterCriticalSection();
        cdcTouchInit();
        leaveCriticalSection(oldIME);
    }
}

void touchApplyCalibration(u16 rawx, u16 rawy, u16 *tpx, u16 *tpy)
{
    s16 px = (rawx * xscale - xoffset + xscale / 2) >> TOUCH_CAL_SHIFT;
    s16 py = (rawy * yscale - yoffset + yscale / 2) >> TOUCH_CAL_SHIFT;

    if (px < 0)
        px = 0;
    else if (px > (SCREEN_WIDTH - 1))
        px = SCREEN_WIDTH - 1;

    if (py < 0)
        py = 0;
    else if (py > (SCREEN_HEIGHT - 1))
        py = SCREEN_HEIGHT - 1;

    *tpx = px;
    *tpy = py;
}

bool touchPenDown(void)
{
    if (cdcIsAvailable())
    {
        int oldIME = enterCriticalSection();
        bool down = cdcTouchPenDown();
        leaveCriticalSection(oldIME);
        return down;
    }
    else
    {
        return tscTouchPenDown();
    }
}

bool touchReadData(touchRawArray *data)
{
    if (cdcIsAvailable())
        return cdcTouchReadData(data);
    else
        return tscTouchReadData(data);
}

void touchReadXY(touchPosition *touchPos)
{
    touchRawArray data;
    if (!touchReadData(&data))
    {
        touchPos->px = 0;
        touchPos->py = 0;
        return;
    }

    touchPos->rawx = libnds_touchMeasurementFilter(data.rawX).value;
    touchPos->rawy = libnds_touchMeasurementFilter(data.rawY).value;
    touchPos->z1 = libnds_touchMeasurementFilter(data.z1).value;
    touchPos->z2 = libnds_touchMeasurementFilter(data.z2).value;

    if (!touchPos->rawx)
    {
        touchPos->px = 0;
        touchPos->py = 0;
        return;
    }

    touchApplyCalibration(touchPos->rawx, touchPos->rawy, &touchPos->px, &touchPos->py);
}
