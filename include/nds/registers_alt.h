// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2005 Michael Noland (joat)
// Copyright (C) 2005 Jason Rogers (dovoto)
// Copyright (C) 2005 Dave Murphy (WinterMute)
// Copyright (C) 2005 Chris Double (doublec)
// Copyright (c) 2024 Adrian "asie" Siekierka

#ifndef LIBNDS_NDS_REGISTERS_ALT_H__
#define LIBNDS_NDS_REGISTERS_ALT_H__

#include <nds/oldnames.h>

#warning Header provided for assistance in porting to new register names, do not use for release code.

// These registers are not yet migrated to the new REG_ standard, as described
// in oldnames.h. This should be done eventually.

#define REG_WIN0H           (*(vu16 *)0x4000040)
#define REG_WIN1H           (*(vu16 *)0x4000042)
#define REG_WIN0V           (*(vu16 *)0x4000044)
#define REG_WIN1V           (*(vu16 *)0x4000046)
#define REG_WININ           (*(vu16 *)0x4000048)
#define REG_WINOUT          (*(vu16 *)0x400004A)

#define REG_WIN0H_SUB       (*(vu16 *)0x4001040)
#define REG_WIN1H_SUB       (*(vu16 *)0x4001042)
#define REG_WIN0V_SUB       (*(vu16 *)0x4001044)
#define REG_WIN1V_SUB       (*(vu16 *)0x4001046)
#define REG_WININ_SUB       (*(vu16 *)0x4001048)
#define REG_WINOUT_SUB      (*(vu16 *)0x400104A)

#define REG_DMA0SAD         (*(vu32 *)0x40000B0)
#define REG_DMA0SAD_L       (*(vu16 *)0x40000B0)
#define REG_DMA0SAD_H       (*(vu16 *)0x40000B2)
#define REG_DMA0DAD         (*(vu32 *)0x40000B4)
#define REG_DMA0DAD_L       (*(vu16 *)0x40000B4)
#define REG_DMA0DAD_H       (*(vu16 *)0x40000B6)
#define REG_DMA0CNT         (*(vu32 *)0x40000B8)
#define REG_DMA0CNT_L       (*(vu16 *)0x40000B8)
#define REG_DMA0CNT_H       (*(vu16 *)0x40000BA)

#define REG_DMA1SAD         (*(vu32 *)0x40000BC)
#define REG_DMA1SAD_L       (*(vu16 *)0x40000BC)
#define REG_DMA1SAD_H       (*(vu16 *)0x40000BE)
#define REG_DMA1DAD         (*(vu32 *)0x40000C0)
#define REG_DMA1DAD_L       (*(vu16 *)0x40000C0)
#define REG_DMA1DAD_H       (*(vu16 *)0x40000C2)
#define REG_DMA1CNT         (*(vu32 *)0x40000C4)
#define REG_DMA1CNT_L       (*(vu16 *)0x40000C4)
#define REG_DMA1CNT_H       (*(vu16 *)0x40000C6)

#define REG_DMA2SAD         (*(vu32 *)0x40000C8)
#define REG_DMA2SAD_L       (*(vu16 *)0x40000C8)
#define REG_DMA2SAD_H       (*(vu16 *)0x40000CA)
#define REG_DMA2DAD         (*(vu32 *)0x40000CC)
#define REG_DMA2DAD_L       (*(vu16 *)0x40000CC)
#define REG_DMA2DAD_H       (*(vu16 *)0x40000CE)
#define REG_DMA2CNT         (*(vu32 *)0x40000D0)
#define REG_DMA2CNT_L       (*(vu16 *)0x40000D0)
#define REG_DMA2CNT_H       (*(vu16 *)0x40000D2)

#define REG_DMA3SAD         (*(vu32 *)0x40000D4)
#define REG_DMA3SAD_L       (*(vu16 *)0x40000D4)
#define REG_DMA3SAD_H       (*(vu16 *)0x40000D6)
#define REG_DMA3DAD         (*(vu32 *)0x40000D8)
#define REG_DMA3DAD_L       (*(vu16 *)0x40000D8)
#define REG_DMA3DAD_H       (*(vu16 *)0x40000DA)
#define REG_DMA3CNT         (*(vu32 *)0x40000DC)
#define REG_DMA3CNT_L       (*(vu16 *)0x40000DC)
#define REG_DMA3CNT_H       (*(vu16 *)0x40000DE)

#define REG_TIME            ((vu16 *)0x4000100)
#define REG_TM0D            (*(vu16 *)0x4000100)
#define REG_TM0CNT          (*(vu16 *)0x4000102)
#define REG_TM1D            (*(vu16 *)0x4000106)
#define REG_TM2D            (*(vu16 *)0x4000108)
#define REG_TM2CNT          (*(vu16 *)0x400010A)
#define REG_TM3D            (*(vu16 *)0x400010C)
#define REG_TM3CNT          (*(vu16 *)0x400010E)

#define REG_SIOMLT_SEND     (*(vu16 *)0x400012A)

#define REG_HS_CTRL         (*(vu16 *)0x4000140)

#define REG_VRAM_A_CR       (*(vu8 *)0x4000240)
#define REG_VRAM_B_CR       (*(vu8 *)0x4000241)
#define REG_VRAM_C_CR       (*(vu8 *)0x4000242)
#define REG_VRAM_D_CR       (*(vu8 *)0x4000243)
#define REG_VRAM_E_CR       (*(vu8 *)0x4000244)
#define REG_VRAM_F_CR       (*(vu8 *)0x4000245)
#define REG_VRAM_G_CR       (*(vu8 *)0x4000246)
#define REG_VRAM_H_CR       (*(vu8 *)0x4000248)
#define REG_VRAM_I_CR       (*(vu8 *)0x4000249)
#define REG_WRAM_CNT        (*(vu8 *)0x4000247)

#define REG_GFX_FIFO        (*(vu32 *)0x4000400)
#define REG_GFX_STATUS      (*(vu32 *)0x4000600)
#define REG_GFX_CONTROL     (*(vu16 *)0x4000060)
#define REG_COLOR           (*(vu32 *)0x4000480)
#define REG_VERTEX16        (*(vu32 *)0x400048C)
#define REG_TEXT_COORD      (*(vu32 *)0x4000488)
#define REG_TEXT_FORMAT     (*(vu32 *)0x40004A8)

#define REG_CLEAR_COLOR     (*(vu32 *)0x4000350)
#define REG_CLEAR_DEPTH     (*(vu16 *)0x4000354)

#define REG_LIGHT_VECTOR    (*(vu32 *)0x40004C8)
#define REG_LIGHT_COLOR     (*(vu32 *)0x40004CC)
#define REG_NORMAL          (*(vu32 *)0x4000484)

#define REG_DIFFUSE_AMBIENT     (*(vu32 *)0x40004C0)
#define REG_SPECULAR_EMISSION   (*(vu32 *)0x40004C4)
#define REG_SHININESS           (*(vu32 *)0x40004D0)

#define REG_POLY_FORMAT     (*(vu32 *)0x40004A4)

#define REG_GFX_BEGIN       (*(vu32 *)0x4000500)
#define REG_GFX_END         (*(vu32 *)0x4000504)
#define REG_GFX_FLUSH       (*(vu32 *)0x4000540)
#define REG_GFX_VIEWPORT    (*(vu32 *)0x4000580)

#define REG_MTX_CONTROL     (*(vu32 *)0x4000440)
#define REG_MTX_PUSH        (*(vu32 *)0x4000444)
#define REG_MTX_POP         (*(vu32 *)0x4000448)
#define REG_MTX_SCALE       (*(vint32 *)0x400046C)
#define REG_MTX_TRANSLATE   (*(vint32 *)0x4000470)
#define REG_MTX_RESTORE     (*(vu32 *)0x4000450)
#define REG_MTX_STORE       (*(vu32 *)0x400044C)
#define REG_MTX_IDENTITY    (*(vu32 *)0x4000454)
#define REG_MTX_LOAD4x4     (*(volatile f32 *)0x4000458)
#define REG_MTX_LOAD4x3     (*(volatile f32 *)0x400045C)
#define REG_MTX_MULT4x4     (*(volatile f32 *)0x4000460)
#define REG_MTX_MULT4x3     (*(volatile f32 *)0x4000464)
#define REG_MTX_MULT3x3     (*(volatile f32 *)0x4000468)

#endif /* LIBNDS_NDS_REGISTERS_ALT_H__ */
