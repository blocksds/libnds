// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2005 Michael Noland (joat)
// Copyright (C) 2005 Jason Rogers (dovoto)
// Copyright (C) 2005 Dave Murphy (WinterMute)

/*! \file paddle.h
   \brief paddle device slot-2 addon support.
*/
#ifndef PADDLE_HEADER_INCLUDE
#define PADDLE_HEADER_INCLUDE

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

#include <nds/ndstypes.h>

/*! \fn bool paddleIsInserted()
    \brief Check for the paddle
    \return true if that's what is in the slot-2
*/
bool paddleIsInserted(void);

/*! \fn void paddleRead()
    \brief Obtain the current paddle state
    \return a u16 containing a 12bit number (fixed point fraction), incrementing for clockwise rotations and decrementing for counterclockwise
*/
u16 paddleRead(void);

//! Resets the paddle device. May change the current value to 0xFFF, 0x000, or 0x001. May perform other unknown internal reset operations. Normally not needed.
void paddleReset(void);


#ifdef __cplusplus
}
#endif

#endif

