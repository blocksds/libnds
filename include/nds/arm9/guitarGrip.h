// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2011 zeromus
// Copyright (C) 2011 Dave Murphy (WinterMute)

/*! \file guitarGrip.h
   \brief guitar grip device slot-2 addon support.
*/
#ifndef GUITARGRIP_HEADER_INCLUDE
#define GUITARGRIP_HEADER_INCLUDE

#ifdef __cplusplus
extern "C" {
#endif

#define GUITARGRIP_GREEN BIT(6)
#define GUITARGRIP_RED BIT(5)
#define GUITARGRIP_YELLOW BIT(4)
#define GUITARGRIP_BLUE BIT(3)


/*! \fn bool guitarGripIsInserted()
    \brief Check for the guitar grip
    \return true if that's what is in the slot-2
*/
bool guitarGripIsInserted(void);

/*! \fn void guitarGripScanKeys()
    \brief Obtain the current guitar grip state.
    Call this function once per main loop to use the guitarGrip functions.
*/
void guitarGripScanKeys(void);

//! Obtains the current guitar grip keys held state
u8 guitarGripKeysHeld(void);

//! Obtains the current guitar grip keys pressed state
u16 guitarGripKeysDown(void);

//! Obtains the current guitar grip keys released state
u16 guitarGripKeysUp(void);


#ifdef __cplusplus
}
#endif

#endif

