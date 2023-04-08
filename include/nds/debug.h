// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2005 Jason Rogers (dovoto)
// Copyright (C) 2005 Dave Murphy (WinterMute)

/*! \file debug.h
\brief Currently only used to send debug messages to NO$GBA debug window

<div class="fileHeader">
On the ARM 9 this functionality is best accessed via the console studio integration.
- \ref console.h "Debug Messages via stdio"

</div>
*/

#ifndef NDS_DEBUG_INCLUDE
#define NDS_DEBUG_INCLUDE

void nocashWrite(const char *message, int len);
/*! \brief Send a message to the no$gba debug window 
\param message The message to send
*/
void nocashMessage(const char *message);

#endif // NDS_DEBUG_INCLUDE

