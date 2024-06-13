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

// This touch filter focuses on removing outlier inputs and reducing noise while
// preserving responsiveness.
//
// Five given inputs are sorted. Of those, the three closest values fitting within
// the provided range are selected, with a small preference for the midpoint. Of
// those, an average is calculated.
//
// This should ensure a consistent readout if >=60% low-noise, valid inputs can be
// identified, even if the inputs are biased in a specific direction.
//
// See also:
//
// - https://dlbeer.co.nz/articles/tsf.html (median, IIR)
// - https://www.ti.com/lit/an/sbaa155a/sbaa155a.pdf (average, weighted average, median)
// - https://www.ti.com/lit/an/slyt209a/slyt209a.pdf (average with out-of-range rejection)

// Compare and swap two values.
static inline void compare_and_swap(u16 *a, u16 *b)
{
    u16 tmp;

    if (*a > *b)
    {
        tmp = *a;
        *a = *b;
        *b = tmp;
    }
}

// Compare two outliers; used internally in libnds_touchFilter.
#define LIBNDS_TOUCH_DIFF_OUTLIERS(a, b) \
    tmp_diff = abs(values[b] - values[a]); \
    if (!tmp_diff) goto location_found; \
    else if (tmp_diff < diff) { \
        v = values + a; \
        diff = tmp_diff; \
    }

u16 libnds_touchFilter(u16 *values, int max_diff)
{
    // Sort using a pre-calculated sorting network.
    // This allows us to quickly check ranges and remove outliers.
    compare_and_swap(&values[0], &values[3]);
    compare_and_swap(&values[1], &values[4]);
    compare_and_swap(&values[0], &values[2]);
    compare_and_swap(&values[1], &values[3]);
    compare_and_swap(&values[0], &values[1]);
    compare_and_swap(&values[2], &values[4]);
    compare_and_swap(&values[1], &values[2]);
    compare_and_swap(&values[3], &values[4]);
    compare_and_swap(&values[2], &values[3]);

    // Find three closest values which are within the specified range.
    // These are the most likely to be the correct read. Prefer the midpoint values.
    u16 *v = values;
    int diff = max_diff + 1;
    int tmp_diff;
    LIBNDS_TOUCH_DIFF_OUTLIERS(1, 3);
    LIBNDS_TOUCH_DIFF_OUTLIERS(0, 2);
    LIBNDS_TOUCH_DIFF_OUTLIERS(2, 4);

    // If no such triplet exists, return an invalid read.
    if (diff > max_diff)
        return 0;

location_found:
    ;
    // Calculate a slightly weighted average; this saves a division.
    u32 value = (((v[0] + v[2]) * 5) + (v[1] * 6)) >> 4;
    // Skip value 0 when returning - libnds assumes it to be an invalid position.
    return value ? value : 1;
}

u32 touchReadTemperature(int *t1, int *t2)
{
    *t1 = tscRead(TSC_MEASURE_TEMP1);
    *t2 = tscRead(TSC_MEASURE_TEMP2);
    return 8490 * (*t2 - *t1) - 273 * 4096;
}

static u16 tscReadFiltered(u32 command, int max_diff)
{
    // Skip the first value.
    u16 values[6];
    tscMeasure(command, values, 6);
    return libnds_touchFilter(values + 1, max_diff);
}

static void touchReadDSMode(touchPosition *touchPos)
{
    int oldIME = enterCriticalSection();

    // Hold ADC on. We're reading at near-full speed, and this
    // may slightly improve read accuracy.
    touchPos->z1 = tscReadFiltered(TSC_MEASURE_Z1 | TSC_POWER_ON, LIBNDS_TOUCH_MAX_DIFF_OTHER);
    touchPos->z2 = tscReadFiltered(TSC_MEASURE_Z2 | TSC_POWER_ON, LIBNDS_TOUCH_MAX_DIFF_OTHER);
    touchPos->rawx = tscReadFiltered(TSC_MEASURE_X | TSC_POWER_ON, LIBNDS_TOUCH_MAX_DIFF_PIXEL);
    touchPos->rawy = tscReadFiltered(TSC_MEASURE_Y | TSC_POWER_ON, LIBNDS_TOUCH_MAX_DIFF_PIXEL);

    // Make an empty read to switch the TSC into power-down mode.
    tscRead(TSC_MEASURE_TEMP1 | TSC_POWER_AUTO);

    leaveCriticalSection(oldIME);

    if (!touchPos->z1) touchPos->z2 = 0;
    else if (!touchPos->z2) touchPos->z1 = 0;

    if (!touchPos->rawx) touchPos->rawy = 0;
    else if (!touchPos->rawy) touchPos->rawx = 0;
}

static s32 xscale, yscale;
static s32 xoffset, yoffset;

void touchInit(void)
{
    xscale = ((PersonalData->calX2px - PersonalData->calX1px) << 19)
             / ((PersonalData->calX2) - (PersonalData->calX1));
    yscale = ((PersonalData->calY2px - PersonalData->calY1px) << 19)
             / ((PersonalData->calY2) - (PersonalData->calY1));

    xoffset = ((PersonalData->calX1 + PersonalData->calX2) * xscale
               - ((PersonalData->calX1px + PersonalData->calX2px) << 19))
              / 2;
    yoffset = ((PersonalData->calY1 + PersonalData->calY2) * yscale
               - ((PersonalData->calY1px + PersonalData->calY2px) << 19))
              / 2;

    if (cdcIsAvailable())
    {
        int oldIME = enterCriticalSection();
        cdcTouchInit();
        leaveCriticalSection(oldIME);
    }
}

bool touchPenDown(void)
{
    bool down;
    int oldIME = enterCriticalSection();

    if (cdcIsAvailable())
        down = cdcTouchPenDown();
    else
        down = !(REG_KEYXY & KEYXY_TOUCH);

    leaveCriticalSection(oldIME);
    return down;
}

void touchReadXY(touchPosition *touchPos)
{
    if (cdcIsAvailable())
        cdcTouchRead(touchPos);
    else
        touchReadDSMode(touchPos);

    if (!touchPos->rawx)
    {
        touchPos->px = 0;
        touchPos->py = 0;
        return;
    }

    s16 px = (touchPos->rawx * xscale - xoffset + xscale / 2) >> 19;
    s16 py = (touchPos->rawy * yscale - yoffset + yscale / 2) >> 19;

    if (px < 0)
        px = 0;
    if (py < 0)
        py = 0;
    if (px > (SCREEN_WIDTH - 1))
        px = SCREEN_WIDTH - 1;
    if (py > (SCREEN_HEIGHT - 1))
        py = SCREEN_HEIGHT - 1;

    touchPos->px = px;
    touchPos->py = py;
}
