// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2023 Antonio Niño Díaz

#ifndef LIBNDS_NDS_CPU_ASM_H__
#define LIBNDS_NDS_CPU_ASM_H__

// This file must only have definitions that can be used from assembly files.

#ifndef BIT
#define BIT(n) (1u << (n))
#endif

#define CPSR_MODE_MASK          (0x1F)

#define CPSR_MODE_USER          (0x10)
#define CPSR_MODE_FIQ           (0x11)
#define CPSR_MODE_IRQ           (0x12)
#define CPSR_MODE_SUPERVISOR    (0x13)
#define CPSR_MODE_ABORT         (0x17)
#define CPSR_MODE_UNDEFINED     (0x1B)
#define CPSR_MODE_SYSTEM        (0x1F)

#define CPSR_FLAG_SIGN          BIT(31) // 0 = Not Signed, 1 = Signed
#define CPSR_FLAG_N             BIT(31)
#define CPSR_FLAG_ZERO          BIT(30) // 0 = Not Zero, 1 = Zero
#define CPSR_FLAG_Z             BIT(30)
#define CPSR_FLAG_CARRY         BIT(29) // 0 = No Carry, 1 = Carry
#define CPSR_FLAG_C             BIT(29)
#define CPSR_FLAG_OVERFLOW      BIT(28) // 0 = No Overflow, 1 = Overflow
#define CPSR_FLAG_V             BIT(28)
#define CPSR_FLAG_IRQ_DIS       BIT(7) // 0 = Enable, 1 = Disable
#define CPSR_FLAG_I             BIT(7)
#define CPSR_FLAG_FIQ_DIS       BIT(6) // 0 = Enable, 1 = Disable
#define CPSR_FLAG_F             BIT(6)
#define CPSR_FLAG_STATE         BIT(5) // 0 = ARM, 1 = THUMB
#define CPSR_FLAG_T             BIT(5)

#endif // LIBNDS_NDS_CPU_ASM_H__
