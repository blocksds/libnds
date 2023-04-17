// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2005 Jason Rogers (dovoto)
// Copyright (C) 2005 Dave Murphy (WinterMute)

/// @file debug.h
///
/// @brief Currently only used to send debug messages to NO$GBA debug window.
///
/// <div class="fileHeader">
/// On the ARM 9 this functionality is best accessed via the console stdio
/// integration.
/// - @ref console.h "Debug Messages via stdio"
/// </div>

#ifndef LIBNDS_NDS_DEBUG_H__
#define LIBNDS_NDS_DEBUG_H__

void nocashWrite(const char *message, int len);

/// Send a message to the no$gba debug window.
///
/// @param message The message to send.
void nocashMessage(const char *message);

#endif // LIBNDS_NDS_DEBUG_H__
