// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2007 Jason Rogers (dovoto)
// Copyright (C) 2007 Dave Murphy (WinterMute)

#ifndef LIBNDS_NDS_ARM9_WINDOW_H__
#define LIBNDS_NDS_ARM9_WINDOW_H__

/// @file nds/arm9/window.h
///
/// @brief Definitions for object and background windowing.

#include <nds/arm9/background.h>
#include <nds/arm9/sassert.h>
#include <nds/arm9/sprite.h>
#include <nds/arm9/video.h>
#include <nds/dma.h>
#include <nds/memory.h>
#include <nds/ndstypes.h>

/// The supported windows
typedef enum {
  WINDOW_0    = DISPLAY_WIN0_ON,    ///< Window 0
  WINDOW_1    = DISPLAY_WIN1_ON,    ///< Window 1
  WINDOW_OBJ  = DISPLAY_SPR_WIN_ON, ///< Object window
  WINDOW_OUT  = BIT(16),            ///< Area outside all windows
} WINDOW;

#define WINDOW_MASK  (WINDOW_0 | WINDOW_1 | WINDOW_OBJ)

/// Enable the specified window(s) (main engine).
///
/// @param w The window to set bounds on (may be ORed together).
static inline void windowEnable(WINDOW w)
{
    REG_DISPCNT |= w & WINDOW_MASK;
}

/// Disable the specified window(s) (main engine).
///
/// @param w The window to set bounds on (may be ORed together).
static inline void windowDisable(WINDOW w)
{
    REG_DISPCNT &= ~(w & WINDOW_MASK);
}

/// Enable the specified window(s) (sub engine).
///
/// @param w The window to set bounds on (may be ORed together).
static inline void windowEnableSub(WINDOW w)
{
    REG_DISPCNT_SUB |= w & WINDOW_MASK;
}

/// Disable the specified window(s) (sub engine).
///
/// @param w The window to set bounds on (may be ORed together).
static inline void windowDisableSub(WINDOW w)
{
    REG_DISPCNT_SUB &= ~(w & WINDOW_MASK);
}

/// Set the windows bounds (main engine).
///
/// @param window The window to set bounds on.
/// @param left The X coordinate of the left hand side of the rectangle.
/// @param top The Y coordinate of the top of the rectangle.
/// @param right The X coordinate of the right hand side of the rectangle.
/// @param bottom The Y coordinate of the bottom of the rectangle.
void windowSetBounds(WINDOW window, u8 left, u8 top, u8 right, u8 bottom);

/// Set the windows bounds (sub engine).
///
/// @param window The window to set bounds on.
/// @param left The X coordinate of the left hand side of the rectangle.
/// @param top The Y coordinate of the top of the rectangle.
/// @param right The X coordinate of the right hand side of the rectangle.
/// @param bottom The Y coordinate of the bottom of the rectangle.
void windowSetBoundsSub(WINDOW window, u8 left, u8 top, u8 right, u8 bottom);

/// Enables the window on the supplied background.
///
/// @param id Background ID returned from bgInit or bgInitSub.
/// @param window The window to enable.
void bgWindowEnable(int id, WINDOW window);

/// Disables the window on the supplied background.
///
/// @param id Background ID returned from bgInit or bgInitSub.
/// @param window The window to disable.
void bgWindowDisable(int id, WINDOW window);

/// Enables the specified OAM window.
///
/// @param oam Must be &oamMain or &oamSub.
/// @param w The window to enable.
void oamWindowEnable(OamState* oam, WINDOW w);

/// Disables the specified OAM window.
///
/// @param oam Must be &oamMain or &oamSub.
/// @param w The window to disable.
void oamWindowDisable(OamState* oam, WINDOW w);

#endif // LIBNDS_NDS_ARM9_WINDOW_H__
