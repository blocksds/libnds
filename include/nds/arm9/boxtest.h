// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2005 Michael Noland (joat)
// Copyright (C) 2005 Jason Rogers (dovoto)
// Copyright (C) 2005 Dave Murphy (WinterMute)
// Copyright (C) 2005 Mike Parks (BigRedPimp)

// Code for performing hardware box test against viewing frustrum

#ifndef LIBNDS_NDS_ARM9_BOXTEST_H__
#define LIBNDS_NDS_ARM9_BOXTEST_H__

#include <nds/arm9/video.h>
#include <nds/arm9/videoGL.h>

/// @file nds/arm9/boxtest.h
///
/// @brief 3D box test dunctions.

#ifdef __cplusplus
extern "C" {
#endif

/// Performs a test to determine if the provided box is in the view frustrum.
///
/// @param x (x, y, z) point of a vertex on the box
/// @param y (x, y, z) point of a vertex on the box
/// @param z (x, y, z) point of a vertex on the box
/// @param height (height, width, depth) describe the size of the box referenced
///               from (x, y, z)
/// @param width (height, width, depth) describe the size of the box referenced
///              from (x, y, z)
/// @param depth (height, width, depth) describe the size of the box referenced
///              from (x, y, z)
/// @return Non zero if any or all of the box is in the view frustum.
int BoxTest(v16 x, v16 y, v16 z, v16 width, v16 height, v16 depth);

/// Performs a test to determine if the provided box is in the view frustum.
///
/// @param x (x, y, z) point of a vertex on the box
/// @param y (x, y, z) point of a vertex on the box
/// @param z (x, y, z) point of a vertex on the box
/// @param width (width, height, depth) describe the size of the box referenced
///              from (x, y, z)
/// @param height (width, height, depth) describe the size of the box referenced
///               from (x, y, z)
/// @param depth (width, height, depth) describe the size of the box referenced
///              from (x, y, z)
/// @return Non zero if any or all of the box is in the view frustum.
static inline int BoxTestf(float x, float y, float z,
                           float width, float height, float depth)
{
    return BoxTest(floattov16(x), floattov16(y), floattov16(z),
                   floattov16(width), floattov16(height), floattov16(depth));
}

/// Performs a test to determine if the provided box is in the view frustum.
///
/// This is asynchronous. BoxTestResult must be called to get the result of this
/// operation.
///
/// @param x (x, y, z) point of a vertex on the box
/// @param y (x, y, z) point of a vertex on the box
/// @param z (x, y, z) point of a vertex on the box
/// @param width (width, height, depth) describe the size of the box referenced
///              from (x, y, z)
/// @param height (width, height, depth) describe the size of the box referenced
///               from (x, y, z)
/// @param depth (width, height, depth) describe the size of the box referenced
///              from (x, y, z)
void BoxTest_Asynch(v16 x, v16 y, v16 z, v16 height, v16 width, v16 depth);

/// Performs a test to determine if the provided box is in the view frustum.
///
/// This is asynchronous. BoxTestResult must be called to get the result of this
/// operation.
///
/// @param x (x, y, z) point of a vertex on the box
/// @param y (x, y, z) point of a vertex on the box
/// @param z (x, y, z) point of a vertex on the box
/// @param width (width, height, depth) describe the size of the box referenced
///              from (x, y, z)
/// @param height (width, height, depth) describe the size of the box referenced
///               from (x, y, z)
/// @param depth (width, height, depth) describe the size of the box referenced
///              from (x, y, z)
static inline void BoxTestf_Asynch(float x, float y, float z,
                                   float width, float height, float depth)
{
    BoxTest_Asynch(floattov16(x), floattov16(y), floattov16(z),
                   floattov16(width), floattov16(height), floattov16(depth));
}

/// Gets the result of the last box test.
///
/// Needed for asynch box test calls.
///
/// @return Non zero if any or all of the box is in the view frustum.
static inline int BoxTestResult(void)
{
    while (GFX_STATUS & GFX_STATUS_TEST_BUSY);

    return GFX_STATUS & GFX_STATUS_TEST_INSIDE;
}

#ifdef __cplusplus
}
#endif

#endif // LIBNDS_NDS_ARM9_BOXTEST_H__
