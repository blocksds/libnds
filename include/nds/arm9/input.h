// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2005 Michael Noland (joat)
// Copyright (C) 2005 Jason Rogers (dovoto)
// Copyright (C) 2005 Dave Murphy (WinterMute)
// Copyright (C) 2005 Christian Auby (DesktopMan)

#ifndef INPUT_HEADER_INCLUDE
#define INPUT_HEADER_INCLUDE

/*! \file
	\brief NDS button and touchscreen input support.
 
 The state of the keypad must be read from hardware into memory using scanKeys() whenever
 you want an updated input state.  After reading, call one of the associated "keys" functions to see
 what event was triggered.  These events are computed as the difference between the current and previous
 key state you read.  It's generally a good idea to scan keys frequently to insure your application's input system
 is responsive.\n
 
 After reading the key state, you will be given an integer representing which keys are in the requested state,
 to mask of specific buttons, use the key masks described in nds/input.h .
 \see nds/input.h available key masks on the Nintendo DS
 
*/

#include <nds/ndstypes.h>
#include <nds/touch.h>
#include <nds/input.h>

#ifdef __cplusplus
extern "C" {
#endif

/*!	\brief Obtains the current keypad state.
	Call this function once per main loop in order to use the keypad functions.
*/
void scanKeys(void);

/*!	\brief Obtains the current keypad state.
	Call this function to get keypad state without affecting state of other key functions (keysUp keysHeld etc...)
*/
uint32 keysCurrent(void);

//!	Obtains the current keypad held state.
uint32 keysHeld(void);

//!	Obtains the current keypad pressed state.
uint32 keysDown(void);

//!	Obtains the current keypad pressed or repeating state.
uint32 keysDownRepeat(void);

/*!	\brief Sets the key repeat parameters.
	\param setDelay Number of %scanKeys calls before keys start to repeat.
	\param setRepeat Number of %scanKeys calls before keys repeat.
*/
void keysSetRepeat( u8 setDelay, u8 setRepeat );

//! Obtains the current keypad released state.
uint32 keysUp(void);



__attribute__ ((deprecated)) touchPosition touchReadXY(void);


/*!
	\brief Obtains the current touchpad state.
	\param data a touchPosition ptr which will be filled by the function.
*/
void touchRead(touchPosition *data);

#ifdef __cplusplus
}
#endif

#endif // INPUT_HEADER_INCLUDE
