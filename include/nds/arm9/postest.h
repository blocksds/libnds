// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2007 Gabe Ghearing (gabebear)

/// @file nds/arm9/postest.h
///
/// @brief Position Test Functions.
///
/// The position test multiplies a given vector by the position matrix and
/// returns the coords(x, y, z, w). The position test is really quick, about 10
/// times faster than a box test.

#ifndef LIBNDS_NDS_ARM9_POSTEST_H__
#define LIBNDS_NDS_ARM9_POSTEST_H__

#include <nds/arm9/video.h>
#include <nds/arm9/videoGL.h>

/// Checks if a position test is being performed.
///
/// @return Returns true if the hardware is currently performing a
/// position/vertex/box test.
static inline bool PosTestBusy(void)
{
    return (GFX_STATUS & BIT(0)) != 0;
}

/// Starts a position test asynchronously.
///
/// @param x Specifies x offset from the current modelview matrix.
/// @param y Specifies y offset from the current modelview matrix.
/// @param z Specifies z offset from the current modelview matrix.
static inline void PosTest_Asynch(v16 x, v16 y, v16 z)
{
    GFX_POS_TEST = VERTEX_PACK(x, y);
    GFX_POS_TEST = z;
}

/// Performs a position test.
///
/// @param x Specifies x offset from the current modelview matrix.
/// @param y Specifies y offset from the current modelview matrix.
/// @param z Specifies z offset from the current modelview matrix.
static inline void PosTest(v16 x, v16 y, v16 z)
{
    PosTest_Asynch(x, y, z);
    while (PosTestBusy());
}

/// Returns the distance from the camera of the last position test.
///
/// @return W magnitude
static inline int32_t PosTestWresult(void)
{
    return GFX_POS_RESULT[3];
}

/// Returns the absolute X position of the last position test (location if the
/// modelview matrix was identity)
///
/// @return Absolute X position
static inline int32_t PosTestXresult(void)
{
    return GFX_POS_RESULT[0];
}

/// Returns the absolute Y position of the last position test (location if the
/// modelview matrix was identity).
///
/// @return Absolute Y position.
static inline int32_t PosTestYresult(void)
{
    return GFX_POS_RESULT[1];
}

/// Returns the absolute Z position of the last position test (location if the
/// modelview matrix was identity).
///
/// @return Absolute Z position.
static inline int32_t PosTestZresult(void)
{
    return GFX_POS_RESULT[2];
}

#endif // LIBNDS_NDS_ARM9_POSTEST_H__
