// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2024 Adrian "asie" Siekierka

// Touch screen filtering for the ARM7

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

// Compare two outliers; used internally in libnds_touchMeasurementFilter.
#define LIBNDS_TOUCH_DIFF_OUTLIERS(a, b) \
    tmp_noise = values[b] - values[a]; \
    if (tmp_noise < result.noisiness) { \
        v = values + a; \
        result.noisiness = tmp_noise; \
        if (!tmp_noise) goto location_found; \
    }

libnds_touchMeasurementFilterResult libnds_touchMeasurementFilter(u16 values[5])
{
    libnds_touchMeasurementFilterResult result = {0, 0xFFFF};

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
    s32 tmp_noise;
    LIBNDS_TOUCH_DIFF_OUTLIERS(1, 3);
    LIBNDS_TOUCH_DIFF_OUTLIERS(0, 2);
    LIBNDS_TOUCH_DIFF_OUTLIERS(2, 4);

location_found:
    ;
    // Calculate a slightly weighted average; this saves a division.
    // (v[0] * 5 + v[1] * 6 + v[2] * 5) / 16
    u16 value = (((v[0] + v[1] + v[2]) * 5) + v[1]) >> 4;
    // Skip value 0 when returning - libnds assumes it to be an invalid position.
    result.value = value ? value : 1;
    return result;
}
