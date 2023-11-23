// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2020 Gericom
// Copyright (C) 2023 Antonio Niño Díaz

#include <nds/arm9/teak/dsp.h>

void dspFifoSend(DSP_PCFG_MEMSEL mem, const u16 *src, bool fixedSrc, u16 dst,
                 bool fixedDst, int length)
{
    if (mem == DSP_PCFG_MEMSEL_PROG)
        return;

    dspSpinWait();

    uint16_t dsp_pcfg = REG_DSP_PCFG;
    dsp_pcfg &= ~(DSP_PCFG_MEMSEL_MASK | DSP_PCFG_AUTOINC);
    dsp_pcfg |=  mem | (fixedDst ? 0 : DSP_PCFG_AUTOINC);
    REG_DSP_PCFG = dsp_pcfg;

    REG_DSP_PADR = dst;

    if (fixedSrc)
    {
        while (length-- > 0)
        {
            dspSpinWait();
            while (REG_DSP_PSTS & DSP_PSTS_WR_FIFO_FULL);
            REG_DSP_PDATA = *src;
        }
    }
    else
    {
        while (length-- > 0)
        {
            dspSpinWait();
            while (REG_DSP_PSTS & DSP_PSTS_WR_FIFO_FULL);
            REG_DSP_PDATA = *src++;
        }
    }

    dspSpinWait();
    REG_DSP_PCFG &= ~(DSP_PCFG_RLEN_MASK | DSP_PCFG_AUTOINC);
}

void dspFifoRecv(DSP_PCFG_MEMSEL mem, u16 src, bool fixedSrc, u16 *dst,
                 bool fixedDst, int length, DSP_PCFG_RLEN lengthMode)
{
    if (mem == DSP_PCFG_MEMSEL_PROG)
        return;

    switch (lengthMode)
    {
        case DSP_PCFG_RLEN_1:
            length = 1;
            break;
        case DSP_PCFG_RLEN_8:
            length = 8;
            break;
        case DSP_PCFG_RLEN_16:
            length = 16;
            break;
        case DSP_PCFG_RLEN_FREE:
            break;
    }

    REG_DSP_PADR = src;
    dspSpinWait();

    uint16_t dsp_pcfg = REG_DSP_PCFG;
    dsp_pcfg &= ~(DSP_PCFG_MEMSEL_MASK | DSP_PCFG_RLEN_MASK | DSP_PCFG_AUTOINC);
    dsp_pcfg |= mem | DSP_PCFG_RSTART | lengthMode | (fixedSrc ? 0 : DSP_PCFG_AUTOINC);
    REG_DSP_PCFG = dsp_pcfg;

    if (fixedDst)
    {
        while (length-- > 0)
        {
            dspSpinWait();
            while (!(REG_DSP_PSTS & DSP_PSTS_RD_FIFO_READY));
            *dst = REG_DSP_PDATA;
        }
    }
    else
    {
        while (length-- > 0)
        {
            dspSpinWait();
            while (!(REG_DSP_PSTS & DSP_PSTS_RD_FIFO_READY));
            *dst++ = REG_DSP_PDATA;
        }
    }

    dspSpinWait();
    REG_DSP_PCFG &= ~(DSP_PCFG_RSTART | DSP_PCFG_RLEN_MASK | DSP_PCFG_AUTOINC);
}
