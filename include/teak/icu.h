// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2020 Gericom
// Copyright (c) 2023 Antonio Niño Díaz

#ifndef LIBTEAK_ICU_H__
#define LIBTEAK_ICU_H__

#include <teak/types.h>

// TODO: Document

#define ICU_IRQ_MASK_SWI(n)             BIT(n) // 0 to 8
#define ICU_IRQ_MASK_TMR1               BIT(9)
#define ICU_IRQ_MASK_TMR0               BIT(10)
#define ICU_IRQ_MASK_BTDMP0             BIT(11)
#define ICU_IRQ_MASK_BTDMP1             BIT(12)
#define ICU_IRQ_MASK_SIO                BIT(13)
#define ICU_IRQ_MASK_APBP               BIT(14)
#define ICU_IRQ_MASK_DMA                BIT(15)

/// ICU Interrupt Pending Flags (R)
#define REG_ICU_IRQ_PENDING             (*(vu16 *)0x8200)
/// ICU Interrupt Acknowledge (W)
#define REG_ICU_IRQ_ACK                 (*(vu16 *)0x8202)
/// ICU Interrupt Manual Trigger (R/W)
#define REG_ICU_IRQ_REQ                 (*(vu16 *)0x8204)
/// ICU Enable Interrupt routing to core interrupt 0 (R/W)
#define REG_ICU_IRQ_INT0                (*(vu16 *)0x8206)
/// ICU Enable Interrupt routing to core interrupt 1 (R/W)
#define REG_ICU_IRQ_INT1                (*(vu16 *)0x8208)
/// ICU Enable Interrupt routing to core interrupt 2 (R/W)
#define REG_ICU_IRQ_INT2                (*(vu16 *)0x820A)
/// ICU Enable Interrupt routing to vectored interrupt (R/W)
#define REG_ICU_IRQ_VINT                (*(vu16 *)0x820C)
/// ICU Interrupt Trigger mode (0=Level, 1=Edge) (R/W)
#define REG_ICU_IRQ_MODE                (*(vu16 *)0x820E)
/// ICU Interrupt Polarity (0=Normal, 1=Invert) (R/W)
#define REG_ICU_IRQ_POLARITY            (*(vu16 *)0x8210)

/// ICU Vectored Interrupt 0..15 Address, bit16-31 (R/W)
#define REG_ICU_VINT_ADDR_HI(x)         (*(vu16 *)(0x8212 + (x) * 4))
/// ICU Vectored Interrupt 0..15 Address, bit0-15 (R/W)
#define REG_ICU_VINT_ADDR_LO(x)         (*(vu16 *)(0x8214 + (x) * 4))

#define ICU_VINT_ADDR_HI(address)       (((address) >> 16) & 0x3)
#define ICU_VINT_ADDR_CTX_SWITCH        BIT(15)

#define ICU_VINT_ADDR_LO(address)       ((address) & 0xFFFF)

/// ICU Interrupt Master Disable (R/W)
#define REG_ICU_IRQ_DISABLE             (*(vu16 *)0x8252)

void icuInit(void);

#endif // LIBTEAK_ICU_H__
