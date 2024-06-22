// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2005 Michael Noland (joat)
// Copyright (C) 2005 Jason Rogers (dovoto)
// Copyright (C) 2005 Dave Murphy (WinterMute)
// Copyright (C) 2005 Chris Double (doublec)
// Copyright (c) 2024 Adrian "asie" Siekierka

#ifndef LIBNDS_NDS_OLDNAMES_H__
#define LIBNDS_NDS_OLDNAMES_H__

#include <nds/ndstypes.h>

// This file contains a list of deprecated libnds names.
// It is intended to be (mostly) machine-parseable.
//
// As such, all entries in it should, if possible, be in one of the following forms:
//
// - #define OLD_NAME NEW_NAME
// - #define OLD_FUNCTION(a,b) NEW_FUNCTION(b,a)
// - typedef new_type old_type;
//
// Versions should be marked by blocks of the following format:
//
// #if BLOCKSDS_STRICT < first_unsupported_version /* first unsupported version+ OR libnds first unsupported version+ */
// ... definitions ...
// #endif
//
// For pre-BlocksDS changes, assume first_unsupported_version is 1.
// Version blocks should always be listed from oldest to newest.

#if BLOCKSDS_STRICT < 1 /* libnds 1.3.0 and below */

// All hardware register defines should be replaced with REG_ for consistency
// and namespacing.
//
// http://forum.gbadev.org/viewtopic.php?t=4993
//
// Replacements may follow gbatek standard naming where applicable; for example:
//
// https://github.com/devkitPro/libnds/commit/5e2ce19b49b60d69adf16fa79b2b440697a5cece
// https://github.com/devkitPro/libnds/commit/fc3e1dffcbeb010179b652f20feb0b382f59b71b
// https://github.com/devkitPro/libnds/commit/e4436bd86ff13dc08e3a910d4acd9eaf681637cd

// Math registers

#define DIV_CR              REG_DIVCNT

#define DIV_NUMERATOR64     REG_DIV_NUMER
#define DIV_NUMERATOR32     REG_DIV_NUMER_L

#define DIV_DENOMINATOR64   REG_DIV_DENOM
#define DIV_DENOMINATOR32   REG_DIV_DENOM_L

#define DIV_RESULT64        REG_DIV_RESULT
#define DIV_RESULT32        REG_DIV_RESULT_L

#define DIV_REMAINDER64     REG_DIVREM_RESULT
#define DIV_REMAINDER32     REG_DIVREM_RESULT_L

#define SQRT_CR             REG_SQRTCNT

#define SQRT_PARAM64        REG_SQRT_PARAM
#define SQRT_PARAM32        REG_SQRT_PARAM_L

#define SQRT_RESULT32       REG_SQRT_RESULT

#define DISPLAY_CR          REG_DISPCNT

#ifdef ARM9
#define WAIT_CR             REG_EXMEMCNT
#else
#define WAIT_CR             REG_EXMEMSTAT
#endif

#define DISP_SR             REG_DISPSTAT
#define DISP_Y              REG_VCOUNT

#define BG0_CR              REG_BG0CNT
#define BG1_CR              REG_BG1CNT
#define BG2_CR              REG_BG2CNT
#define BG3_CR              REG_BG3CNT

#define BG0_X0              REG_BG0HOFS
#define BG0_Y0              REG_BG0VOFS
#define BG1_X0              REG_BG1HOFS
#define BG1_Y0              REG_BG1VOFS
#define BG2_X0              REG_BG2HOFS
#define BG2_Y0              REG_BG2VOFS
#define BG3_X0              REG_BG3HOFS
#define BG3_Y0              REG_BG3VOFS

#define BG2_XDX             REG_BG2PA
#define BG2_XDY             REG_BG2PB
#define BG2_YDX             REG_BG2PC
#define BG2_YDY             REG_BG2PD
#define BG2_CX              REG_BG2X
#define BG2_CY              REG_BG2Y

#define BG3_XDX             REG_BG3PA
#define BG3_XDY             REG_BG3PB
#define BG3_YDX             REG_BG3PC
#define BG3_YDY             REG_BG3PD
#define BG3_CX              REG_BG3X
#define BG3_CY              REG_BG3Y

#define SERIAL_CR           REG_SPICNT
#define SERIAL_DATA         REG_SPIDATA
#define SIO_CR              REG_SIOCNT
#define R_CR                REG_RCNT

#define DISP_CAPTURE        REG_DISPCAPCNT

#define BRIGHTNESS          REG_MASTER_BRIGHT
#define SUB_BRIGHTNESS      REG_MASTER_BRIGHT_SUB

#define SUB_DISPLAY_CR      REG_DISPCNT_SUB

#define SUB_BG0_CR          REG_BG0CNT_SUB
#define SUB_BG1_CR          REG_BG1CNT_SUB
#define SUB_BG2_CR          REG_BG2CNT_SUB
#define SUB_BG3_CR          REG_BG3CNT_SUB

#define SUB_BG0_X0          REG_BG0HOFS_SUB
#define SUB_BG0_Y0          REG_BG0VOFS_SUB
#define SUB_BG1_X0          REG_BG1HOFS_SUB
#define SUB_BG1_Y0          REG_BG1VOFS_SUB
#define SUB_BG2_X0          REG_BG2HOFS_SUB
#define SUB_BG2_Y0          REG_BG2VOFS_SUB
#define SUB_BG3_X0          REG_BG3HOFS_SUB
#define SUB_BG3_Y0          REG_BG3VOFS_SUB

#define SUB_BG2_XDX         REG_BG2PA_SUB
#define SUB_BG2_XDY         REG_BG2PB_SUB
#define SUB_BG2_YDX         REG_BG2PC_SUB
#define SUB_BG2_YDY         REG_BG2PD_SUB
#define SUB_BG2_CX          REG_BG2X_SUB
#define SUB_BG2_CY          REG_BG2Y_SUB

#define SUB_BG3_XDX         REG_BG3PA_SUB
#define SUB_BG3_XDY         REG_BG3PB_SUB
#define SUB_BG3_YDX         REG_BG3PC_SUB
#define SUB_BG3_YDY         REG_BG3PD_SUB
#define SUB_BG3_CX          REG_BG3X_SUB
#define SUB_BG3_CY          REG_BG3Y_SUB

#define KEYS                REG_KEYINPUT
#define KEYS_CR             REG_KEYCNT

#define IE                  REG_IE
#define IF                  REG_IF
#define IME                 REG_IME

#define POWER_CR            REG_POWERCNT
#endif

#if BLOCKSDS_STRICT < 1 /* libnds 1.3.3+ */
#define SOUND_CR            REG_SOUNDCNT
#define SOUND_MASTER_VOL    REG_MASTER_VOLUME
#define SOUND_BIAS          REG_SOUNDBIAS

#define BLEND_CR            REG_BLDCNT
#define BLEND_AB            REG_BLDALPHA
#define BLEND_Y             REG_BLDY

#define SUB_BLEND_CR        REG_BLDCNT_SUB
#define SUB_BLEND_AB        REG_BLDALPHA_SUB
#define SUB_BLEND_Y         REG_BLDY_SUB

#define REG_BLDMOD_SUB      REG_BLDCNT_SUB
#define REG_COLV_SUB        REG_BLDALPHA_SUB
#define REG_COLY_SUB        REG_BLDY_SUB
#endif

#if BLOCKSDS_STRICT < 1 /* libnds 1.3.5+ */
#define MOSAIC_CR           REG_MOSAIC
#define SUB_MOSAIC_CR       REG_MOSAIC_SUB
#endif

#if BLOCKSDS_STRICT < 1 /* libnds 1.4.5+ */
#define CARD_CR1            REG_AUXSPICNT
#define CARD_CR1H           REG_AUXSPICNTH
#define CARD_CR2            REG_ROMCTRL
#define CARD_EEPDATA        REG_AUXSPIDATA
#endif

#if BLOCKSDS_STRICT < 1 /* libnds 1.7.0+ */
#define CARD_COMMAND        REG_CARD_COMMAND
#define CARD_DATA_RD        REG_CARD_DATA_RD
#define CARD_1B0            REG_CARD_1B0
#define CARD_1B4            REG_CARD_1B4
#define CARD_1B8            REG_CARD_1B8
#define CARD_1BA            REG_CARD_1BA
#endif

#if BLOCKSDS_STRICT < 300 /* 0.3.0+ */
#define IRQ_NETWORK         IRQ_RTC ///< \deprecated Replaced in BlocksDS 0.3.0 by IRQ_RTC.
#define FIFO_SDMMC          FIFO_STORAGE ///< \deprecated Replaced in BlocksDS 0.3.0 by FIFO_STORAGE.
#endif

#if BLOCKSDS_STRICT < 10300 /* 1.3.0+ */
#define PUnpackStruct       TUnpackStruct*
__attribute__((deprecated)) typedef void (* fp)(void);
#define RTCtime             rtcTimeAndDate
#define vramSetMainBanks    vramSetPrimaryBanks
#define vramRestoreMainBanks vramRestorePrimaryBanks

#ifdef ARM7
#define RTC_CR              REG_RTCCNT
#define RTC_CR8             REG_RTCCNT8
#define SerialWaitBusy      spiWaitBusy
#define touchRead           tscRead
#define touchReadTemperature tscReadTemperature
#endif
#ifdef ARM9
#define HALT_CR             REG_HALTCNT
#endif
#endif

#endif // LIBNDS_NDS_OLDNAMES_H__
