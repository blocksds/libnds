// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2020 Gericom
// Copyright (C) 2023 Antonio Niño Díaz

#include <teak/btdmp.h>
#include <teak/icu.h>

void btdmpSetupOutputSpeakers(int channel, int irq_index)
{
    if (channel == 0)
    {
        REG_BTDMP_TRANSMIT_UNK20(0) = 0xF | (1 << 8); // Transmit
        //REG_BTDMP_TRANSMIT_UNK20(0) = 0xD | (2 << 8); // Receive

        // 1004h selects ExternalClk (32.73kHz or 47.61kHz, as selected in
        // REG_SNDEXCNT on the ARM7 (2x16bit stereo)
        REG_BTDMP_TRANSMIT_UNK22(0) = 0x1004;

        REG_BTDMP_TRANSMIT_UNK24(0) = 4;
        REG_BTDMP_TRANSMIT_UNK26(0) = 0x21;
        REG_BTDMP_TRANSMIT_UNK28(0) = 0;
        REG_BTDMP_TRANSMIT_UNK2A(0) = 0;

        btdmpDisableTransmit(0);
        btdmpFlushTransmitFifo(0);

        if (irq_index == 0)
            REG_ICU_IRQ_INT0 = ICU_IRQ_MASK_BTDMP0;
        else if (irq_index == 1)
            REG_ICU_IRQ_INT1 = ICU_IRQ_MASK_BTDMP0;
        else //if (irq_index == 2)
            REG_ICU_IRQ_INT2 = ICU_IRQ_MASK_BTDMP0;

        REG_ICU_IRQ_MODE &= ~ICU_IRQ_MASK_BTDMP0;
        REG_ICU_IRQ_POLARITY &= ~ICU_IRQ_MASK_BTDMP0;
        REG_ICU_IRQ_DISABLE &= ~ICU_IRQ_MASK_BTDMP0;
        REG_ICU_IRQ_ACK = ICU_IRQ_MASK_BTDMP0;

        btdmpEnableTransmit(0);
    }
    else
    {
        REG_BTDMP_TRANSMIT_UNK20(1) = 0xF | (1 << 8); // Transmit
        REG_BTDMP_TRANSMIT_UNK22(1) = 0x1004;
        REG_BTDMP_TRANSMIT_UNK24(1) = 4;
        REG_BTDMP_TRANSMIT_UNK26(1) = 0x21;
        REG_BTDMP_TRANSMIT_UNK28(1) = 0;
        REG_BTDMP_TRANSMIT_UNK2A(1) = 0;

        btdmpDisableTransmit(1);
        btdmpFlushTransmitFifo(1);

        if (irq_index == 0)
            REG_ICU_IRQ_INT0 = ICU_IRQ_MASK_BTDMP1;
        else if (irq_index == 1)
            REG_ICU_IRQ_INT1 = ICU_IRQ_MASK_BTDMP1;
        else //if (irq_index == 2)
            REG_ICU_IRQ_INT2 = ICU_IRQ_MASK_BTDMP1;

        REG_ICU_IRQ_MODE &= ~ICU_IRQ_MASK_BTDMP1;
        REG_ICU_IRQ_POLARITY &= ~ICU_IRQ_MASK_BTDMP1;
        REG_ICU_IRQ_DISABLE &= ~ICU_IRQ_MASK_BTDMP1;
        REG_ICU_IRQ_ACK = ICU_IRQ_MASK_BTDMP1;

        btdmpEnableTransmit(1);
    }
}
