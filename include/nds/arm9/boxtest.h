// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2005 Michael Noland (joat)
// Copyright (C) 2005 Jason Rogers (dovoto)
// Copyright (C) 2005 Dave Murphy (WinterMute)
// Copyright (C) 2005 Mike Parks (BigRedPimp)

// Code for performing hardware box test against viewing frustrum

#ifndef BOX_TEST_INCLUDE
#define BOX_TEST_INCLUDE

#include "nds/arm9/video.h"
#include "nds/arm9/videoGL.h"

/*! \file boxtest.h
	\brief Box Test Functions.
*/

#ifdef __cplusplus
extern "C" {
#endif


/*!	\brief Performs a test to determine if the provided box is in the view frustrum.
	\param x (x, y, z) point of a vertex on the box
	\param y (x, y, z) point of a vertex on the box
	\param z (x, y, z) point of a vertex on the box
	\param height (height, width, depth) describe the size of the box referenced from (x, y, z)
	\param width (height, width, depth) describe the size of the box referenced from (x, y, z)
	\param depth (height, width, depth) describe the size of the box referenced from (x, y, z)

	\return non zero if any or all of the box is in the view frustum.
*/
int BoxTest(v16 x, v16 y, v16 z, v16 width, v16 height, v16 depth);

/*!	\brief Performs a test to determine if the provided box is in the view frustum.
	\param x (x, y, z) point of a vertex on the box
	\param y (x, y, z) point of a vertex on the box
	\param z (x, y, z) point of a vertex on the box
	\param width (width, height, depth) describe the size of the box referenced from (x, y, z)
	\param height (width, height, depth) describe the size of the box referenced from (x, y, z)
	\param depth (width, height, depth) describe the size of the box referenced from (x, y, z)

	\return non zero if any or all of the box is in the view frustum.
*/
int BoxTestf(float x, float y, float z, float width, float height, float depth);

/*!	\brief Performs a test to determine if the provided box is in the view frustum.
	Performs a test to determine if the provided box is in the view frustum.
	BoxTestResult must be called to get the result of this operation.

	\param x (x, y, z) point of a vertex on the box
	\param y (x, y, z) point of a vertex on the box
	\param z (x, y, z) point of a vertex on the box
	\param width (width, height, depth) describe the size of the box referenced from (x, y, z)
	\param height (width, height, depth) describe the size of the box referenced from (x, y, z)
	\param depth (width, height, depth) describe the size of the box referenced from (x, y, z)
*/
void BoxTest_Asynch(v16 x, v16 y, v16 z, v16 height, v16 width, v16 depth);

/*!	\brief Performs a test to determine if the provided box is in the view frustum.
	Performs a test to determine if the provided box is in the view frustum.
	BoxTestResult must be called to get the result of this operation.

	\param x (x, y, z) point of a vertex on the box
	\param y (x, y, z) point of a vertex on the box
	\param z (x, y, z) point of a vertex on the box
	\param width (width, height, depth) describe the size of the box referenced from (x, y, z)
	\param height (width, height, depth) describe the size of the box referenced from (x, y, z)
	\param depth (width, height, depth) describe the size of the box referenced from (x, y, z)
*/
void BoxTestf_Asynch(float x, float y, float z, float width, float height, float depth);

/*!	\brief Gets the result of the last box test. Needed for asynch box test calls.
	\return non zero if any or all of the box is in the view frustum.
*/
int BoxTestResult(void);

#ifdef __cplusplus
}
#endif
#endif
