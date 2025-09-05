// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2005 Michael Noland (joat)
// Copyright (C) 2005 Jason Rogers (dovoto)
// Copyright (C) 2005 Dave Murphy (WinterMute)
// Copyright (C) 2023 Antonio Niño Díaz

/// @file nds.h
///
/// @brief The global include file for NDS applications.
///
/// @mainpage Libnds documentation
///
/// @section intro Introduction
/// Welcome to the libnds reference documentation.
///
/// @section video_2D_api 2D engine API
/// - @ref nds/arm9/video.h "General video"
/// - @ref nds/arm9/background.h "2D Background Layers"
/// - @ref nds/arm9/sprite.h "2D Sprites"
/// - @ref nds/arm9/window.h "Sprite and background windows"
///
/// @section video_3D_api 3D engine API
/// - @ref nds/arm9/videoGL.h "OpenGL (ish)"
/// - @ref nds/arm9/boxtest.h "Box Test"
/// - @ref nds/arm9/postest.h "Position test"
/// - @ref gl2d.h "GL2D: 2D graphics using 3D"
///
/// @section audio_api Audio API
/// - @ref nds/arm9/sound.h "Simple Sound Engine"
///
/// @section math_api Math
/// - @ref nds/arm9/math.h "Hardware Assisted Math"
/// - @ref nds/arm9/trig_lut.h "Fixed point trigenometry functions"
///
/// @section memory_api Memory
/// - @ref nds/memory.h "General memory definitions"
/// - @ref nds/memory.h "NDS and GBA header structure"
/// - @ref nds/dma.h "Direct Memory Access"
/// - @ref nds/ndma.h "DSi New Direct Memory Access"
/// - @ref nds/nwram.h "DSi New WRAM"
///
/// @section filesystem_api Storage Access
/// - @ref nds/card.h "Slot-1 common access functions"
/// - @ref nds/arm9/card.h "Slot-1 ARM9 access functions"
/// - @ref fat.h "Simple replacement of libfat"
/// - @ref filesystem.h "NitroFS, filesystem embedded in a NDS ROM"
/// - @ref nds/arm9/sdmmc.h "ARM9 SDMMC Module"
/// - @ref nds/arm7/nand_crypto.h "ARM7 Low-level NAND cryptographic helper functions"
///
/// @section system_api System
/// - @ref nds/ndstypes.h "Custom DS types"
/// - @ref nds/system.h "Hardware Initilization"
/// - @ref nds/bios.h "Bios"
/// - @ref nds/arm9/cache.h "ARM9 Cache"
/// - @ref nds/interrupts.h "Interrupts"
/// - @ref nds/fifocommon.h "FIFO"
/// - @ref nds/timers.h "Timers"
/// - @ref nds/device_list.h "Structures for the Device List protocol used by the DSi/3DS launchers"
///
/// @section multithreading_api Multithreading
/// - @ref nds/cothread.h "Cooperative multithreading"
///
/// @section dynamic_library_api Dynamic libraries
/// - @ref dlfcn.h "Helpers to load dynamic libraries"
///
/// @section user_io_api User Input/ouput
/// - @ref nds/arm9/input.h "Keypad and touch pad"
/// - @ref nds/arm9/keyboard.h "Keyboard"
/// - @ref nds/arm9/console.h "Console and Debug Printing"
/// - @ref nds/touch.h "Touch screen definitions"
/// - @ref nds/input.h "Input definitions"
///
/// @section dsi_camera DSi Camera
/// - @ref nds/arm9/camera.h "High-level ARM9 DSi camera functions"
/// - @ref nds/arm7/camera.h "Low-level ARM7 DSi camera functions"
///
/// @section dsp_api DSi Teak DSP Utilities
/// - @ref nds/arm9/teak/dsp.h "DSP general utilities"
/// - @ref nds/arm9/teak/fifo.h "DSP <-> ARM9 FIFO transfer utilities"
/// - @ref nds/arm9/teak/tlf.h "TLF format description"
///
/// @section utility_api Utility
/// - @ref nds/arm9/image.h "Image Manipulation"
/// - @ref nds/arm9/grf.h "GRF file loader"
/// - @ref nds/arm9/pcx.h "PCX file loader"
/// - @ref nds/arm9/dynamicArray.h "General Purpose dynamic array implementation"
/// - @ref nds/arm9/linkedlist.h "General purpose linked list implementation"
/// - @ref nds/decompress.h "Decompression"
/// - @ref nds/rsa.h "DSi RSA functions"
/// - @ref nds/sha1.h "DSi SHA1 functions"
/// - @ref nds/utf.h "UTF functions"
///
/// @section peripheral_api Custom Peripherals
/// - @ref nds/arm9/peripherals/slot2.h "Slot-2 peripheral detection, external RAM"
/// - @ref nds/arm9/peripherals/slot2gyro.h "Slot-2 gyro sensor"
/// - @ref nds/arm9/peripherals/slot2solar.h "Slot-2 solar sensor"
/// - @ref nds/arm9/peripherals/slot2tilt.h "Slot-2 tilt sensor"
/// - @ref nds/arm9/rumble.h "Slot-2 Rumble"
/// - @ref nds/arm9/ndsmotion.h "DS Motion Card/Pak"
/// - @ref nds/arm9/piano.h "DS Easy Piano"
/// - @ref nds/arm9/paddle.h "Slot-2 Paddle controller"
/// - @ref nds/arm9/guitarGrip.h "Slot-2 Guitar Grip controller"
///
/// @section arm7 ARM7 modules
/// - @ref nds/arm7/clock.h "RTC utilities"
/// - @ref nds/arm7/console.h "Console to that sends messages to the ARM9 easily"
/// - @ref nds/arm7/input.h "Keypad and touch pad ARM7 helpers"
/// - @ref nds/arm7/audio.h "Audio and microphone helpers"
/// - @ref nds/arm7/touch.h "Touch screen helpers"
/// - @ref nds/arm7/serial.h "SPI bus controller"
/// - @ref nds/arm7/firmware.h "Firmware flash access helpers"
/// - @ref nds/arm7/tsc.h "DS Touchscreen/Sound controller helpers"
/// - @ref nds/arm7/tmio.h "DSi TMIO ARM7 module"
/// - @ref nds/arm7/gpio.h "DSi GPIO ARM7 definitions and helpers"
///
/// @section debug_api Debugging
/// - @ref nds/arm9/console.h "Debug via printf to DS screen or NO$GBA"
/// - @ref nds/arm9/sassert.h "Simple assert"
/// - @ref nds/debug.h "Send message to NO$GBA"
/// - @ref nds/exceptions.h "Exception handling"

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
#include <nds/decompress.h>
#include <nds/device_list.h>
#include <nds/dma.h>
#include <nds/exceptions.h>
#include <nds/fifocommon.h>
#include <nds/input.h>
#include <nds/interrupts.h>
#include <nds/ipc.h>
#include <nds/libversion.h>
#include <nds/memory.h>
#include <nds/ndma.h>
#include <nds/ndstypes.h>
#include <nds/nwram.h>
#include <nds/rsa.h>
#include <nds/sha1.h>
#include <nds/system.h>
#include <nds/timers.h>
#include <nds/touch.h>
#include <nds/utf.h>

#ifdef ARM9
#    include <nds/arm9/background.h>
#    include <nds/arm9/boxtest.h>
#    include <nds/arm9/cache.h>
#    include <nds/arm9/camera.h>
#    include <nds/arm9/console.h>
#    include <nds/arm9/dynamicArray.h>
#    include <nds/arm9/guitarGrip.h>
#    include <nds/arm9/image.h>
#    include <nds/arm9/input.h>
#    include <nds/arm9/keyboard.h>
#    include <nds/arm9/linkedlist.h>
#    include <nds/arm9/math.h>
#    include <nds/arm9/ndsmotion.h>
#    include <nds/arm9/paddle.h>
#    include <nds/arm9/grf.h>
#    include <nds/arm9/pcx.h>
#    include <nds/arm9/piano.h>
#    include <nds/arm9/rumble.h>
#    include <nds/arm9/sassert.h>
#    include <nds/arm9/sdmmc.h>
#    include <nds/arm9/sound.h>
#    include <nds/arm9/sprite.h>
#    include <nds/arm9/trig_lut.h>
#    include <nds/arm9/video.h>
#    include <nds/arm9/videoGL.h>
#    include <nds/arm9/window.h>
#    include <nds/arm9/peripherals/slot2.h>
#    include <nds/arm9/peripherals/slot2gyro.h>
#    include <nds/arm9/peripherals/slot2solar.h>
#    include <nds/arm9/peripherals/slot2tilt.h>
#    include <nds/arm9/teak/dsp.h>
#    include <nds/arm9/teak/fifo.h>
#    include <nds/arm9/teak/tlf.h>
#endif // ARM9

#ifdef ARM7
#    include <nds/arm7/aes.h>
#    include <nds/arm7/audio.h>
#    include <nds/arm7/camera.h>
#    include <nds/arm7/clock.h>
#    include <nds/arm7/codec.h>
#    include <nds/arm7/console.h>
#    include <nds/arm7/firmware.h>
#    include <nds/arm7/gpio.h>
#    include <nds/arm7/i2c.h>
#    include <nds/arm7/input.h>
#    include <nds/arm7/nand_crypto.h>
#    include <nds/arm7/sdmmc.h>
#    include <nds/arm7/serial.h>
#    include <nds/arm7/tmio.h>
#    include <nds/arm7/touch.h>
#    include <nds/arm7/tsc.h>
#endif // ARM7

#ifdef __cplusplus
}
#endif

#endif // LIBNDS_NDS_H__
