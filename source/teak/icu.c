// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2020 Gericom

#include <teak/teak.h>

void icuInit(void)
{
    cpuDisableIrqs();
    REG_ICU_IRQ_DISABLE = 0xFFFF;
    REG_ICU_IRQ_ACK = 0xFFFF;
    REG_ICU_IRQ_INT0 = 0;
    REG_ICU_IRQ_INT1 = 0;
    REG_ICU_IRQ_INT2 = 0;
    REG_ICU_IRQ_VINT = 0;
    REG_ICU_IRQ_MODE = 0;
    REG_ICU_IRQ_POLARITY = 0;
    cpuDisableInt0();
    cpuDisableInt1();
    cpuDisableInt2();
    cpuDisableVInt();
    cpuEnableIrqs();
}
