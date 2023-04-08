// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2005 Michael Noland (joat)
// Copyright (C) 2005 Jason Rogers (dovoto)
// Copyright (C) 2005 Dave Murphy (WinterMute)
// Copyright (C) 2005 Mike Parks (BigRedPimp)

/*! \file rumble.h
    \brief nds rumble option pak support.
*/
#ifndef RUMBLE_HEADER_INCLUDE
#define RUMBLE_HEADER_INCLUDE

#ifdef __cplusplus
extern "C" {
#endif

#define RUMBLE_PAK		(*(vuint16 *)0x08000000)
#define WARIOWARE_PAK		(*(vuint16 *)0x080000C4)
#define WARIOWARE_ENABLE	(*(vuint16 *)0x080000C6)

typedef enum {
   RUMBLE_TYPE_UNKNOWN,
   RUMBLE_TYPE_NONE,
   RUMBLE_TYPE_PAK, /* DS Rumble Pak */
   RUMBLE_TYPE_GBA, /* rumble included as part of GBA game cartridges */
   RUMBLE_TYPE_MAGUKIDDO /* rumble/sensor cartridge bundled with Magukiddo */
} RUMBLE_TYPE;

void rumbleInit(void);
RUMBLE_TYPE rumbleGetType(void);
void rumbleSetType(RUMBLE_TYPE type);

/*! \fn bool isRumbleInserted(void);
	\brief Check for rumble option pak.
	\return true if the cart in the GBA slot is a Rumble option pak.
*/
bool isRumbleInserted(void);

/*! \fn void setRumble(bool position);
	\param position Alternates position of the actuator in the pak
	\brief Fires the rumble actuator.
*/
void setRumble(bool position);

#ifdef __cplusplus
}
#endif

#endif
