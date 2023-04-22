// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2005 Michael Noland (joat)
// Copyright (C) 2005 Jason Rogers (dovoto)
// Copyright (C) 2005 Dave Murphy (WinterMute)

/// @file nds.h
///
/// @brief The master include file for NDS applications.
///
/// @mainpage Libnds Documentation
///
/// @section intro Introduction
/// Welcome to the libnds reference documentation.
///
/// @section video_2D_api 2D engine API
/// - @ref video.h "General video"
/// - @ref background.h "2D Background Layers"
/// - @ref sprite.h "2D Sprites"
///
/// @section video_3D_api 3D engine API
/// - @ref videoGL.h "OpenGL (ish)"
/// - @ref boxtest.h "Box Test"
/// - @ref postest.h "Position test"
/// - @ref gl2d.h "Simple DS 2D rendering using the 3d core"
///
/// @section audio_api Audio API
/// - @ref sound.h "Simple Sound Engine"
/// - <a href="https://maxmod.devkitpro.org/ref">Maxmod</a>
///
/// @section math_api Math
/// - @ref math.h "Hardware Assisted Math"
/// - @ref trig_lut.h "Fixed point trigenometry functions"
///
/// @section memory_api Memory
/// - @ref memory.h "General memory definitions"
/// - @ref memory.h "nds and gba header structure"
/// - @ref dma.h "Direct Memory Access"
/// - @ref ndma.h "DSi New Direct Memory Access"
///
/// @section filesystem_api Storage Access
/// - @ref card.h "Slot-1 access functions"
/// - @ref fat.h "Simple replacement of libfat"
/// - @ref filesystem.h "NitroFAT, filesystem embedded in a NDS ROM"
///
/// @section system_api System
/// - @ref ndstypes.h "Custom DS types"
/// - @ref system.h "Hardware Initilization"
/// - @ref bios.h "Bios"
/// - @ref cache.h "ARM 9 Cache"
/// - @ref interrupts.h "Interrupts"
/// - @ref fifocommon.h "FIFO"
/// - @ref timers.h "Timers"
///
/// @section multithreading_api Multithreading
/// - @ref cothread.h "Cooperative multithreading"
///
/// @section user_io_api User Input/ouput
/// - @ref arm9/input.h "Keypad and Touch pad"
/// - @ref keyboard.h "Keyboard"
/// - @ref console.h "Console and Debug Printing"
///
/// @section utility_api Utility
/// - @ref decompress.h "Decompression"
/// - @ref image.h "Image Manipulation"
/// - @ref pcx.h "PCX file loader"
/// - @ref dynamicArray.h "General Purpose dynamic array implementation"
/// - @ref linkedlist.h "General purpose linked list implementation"
///
/// @section peripheral_api Custom Peripherals
/// - @ref rumble.h "Rumble Pack"
/// - @ref ndsmotion.h "DS Motion Pack"
/// - @ref piano.h "DS Easy Piano Controller"
///
/// @section debug_api Debugging
/// - @ref console.h "Debug via printf to DS screen or NO$GBA"
/// - @ref debug.h "Send message to NO$GBA"
/// - @ref sassert.h "Simple assert"
/// - @ref exceptions.h "ARM9 exception handler"

#ifndef LIBNDS_NDS_H__
#define LIBNDS_NDS_H__

#ifndef ARM7
#    ifndef ARM9
#        error Either ARM7 or ARM9 must be defined
#    endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include <nds/bios.h>
#include <nds/camera.h>
#include <nds/card.h>
#include <nds/cothread.h>
#include <nds/cpu.h>
#include <nds/debug.h>
#include <nds/dma.h>
#include <nds/fifocommon.h>
#include <nds/input.h>
#include <nds/interrupts.h>
#include <nds/ipc.h>
#include <nds/libversion.h>
#include <nds/memory.h>
#include <nds/ndma.h>
#include <nds/ndstypes.h>
#include <nds/sha1.h>
#include <nds/system.h>
#include <nds/timers.h>
#include <nds/touch.h>

#ifdef ARM9
#    include <nds/arm9/background.h>
#    include <nds/arm9/boxtest.h>
#    include <nds/arm9/cache.h>
#    include <nds/arm9/camera.h>
#    include <nds/arm9/console.h>
#    include <nds/arm9/decompress.h>
#    include <nds/arm9/dynamicArray.h>
#    include <nds/arm9/exceptions.h>
#    include <nds/arm9/guitarGrip.h>
#    include <nds/arm9/image.h>
#    include <nds/arm9/input.h>
#    include <nds/arm9/keyboard.h>
#    include <nds/arm9/linkedlist.h>
#    include <nds/arm9/math.h>
#    include <nds/arm9/nand.h>
#    include <nds/arm9/paddle.h>
#    include <nds/arm9/pcx.h>
#    include <nds/arm9/piano.h>
#    include <nds/arm9/rumble.h>
#    include <nds/arm9/sassert.h>
#    include <nds/arm9/sound.h>
#    include <nds/arm9/sprite.h>
#    include <nds/arm9/trig_lut.h>
#    include <nds/arm9/video.h>
#    include <nds/arm9/videoGL.h>
#    include <nds/arm9/window.h>
#endif // ARM9

#ifdef ARM7
#    include <nds/arm7/aes.h>
#    include <nds/arm7/audio.h>
#    include <nds/arm7/camera.h>
#    include <nds/arm7/clock.h>
#    include <nds/arm7/codec.h>
#    include <nds/arm7/i2c.h>
#    include <nds/arm7/input.h>
#    include <nds/arm7/sdmmc.h>
#    include <nds/arm7/serial.h>
#    include <nds/arm7/touch.h>
#endif // ARM7

#ifdef __cplusplus
}
#endif

#endif // LIBNDS_NDS_H__
