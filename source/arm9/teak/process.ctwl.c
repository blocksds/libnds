// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2020 Gericom
// Copyright (c) 2023 Antonio Niño Díaz

#include <assert.h>
#include <string.h>

#include <nds/arm9/teak/dsp.h>
#include <nds/arm9/teak/tlf.h>
#include <nds/memory.h>
#include <nds/nwram.h>

static u16 _slotB;
static u16 _slotC;
static int _codeSegs;
static int _dataSegs;
static u8 _codeSlots[NWRAM_BC_SLOT_COUNT];
static u8 _dataSlots[NWRAM_BC_SLOT_COUNT];

static void *DspToArm9Address(bool isCodePtr, u32 addr)
{
    addr = DSP_MEM_ADDR_TO_CPU(addr);
    int seg = addr >> NWRAM_BC_SLOT_SHIFT;
    int offs = addr - (seg << NWRAM_BC_SLOT_SHIFT);
    int slot = isCodePtr ? _codeSlots[seg] : _dataSlots[seg];
    return (char *)nwramGetBlockAddress(isCodePtr ? NWRAM_BLOCK_B : NWRAM_BLOCK_C)
            + slot * NWRAM_BC_SLOT_SIZE + offs;
}

static bool SetMemoryMapping(bool isCode, u32 addr, u32 len, bool toDsp)
{
    addr = DSP_MEM_ADDR_TO_CPU(addr);
    len = DSP_MEM_ADDR_TO_CPU(len < 1 ? 1 : len);
    int segBits = isCode ? _codeSegs : _dataSegs;
    int start = addr >> NWRAM_BC_SLOT_SHIFT;
    int end = (addr + len - 1) >> NWRAM_BC_SLOT_SHIFT;
    for (int i = start; i <= end; i++)
    {
        if ((segBits & (1 << i)) == 0)
            continue;
        int slot = isCode ? _codeSlots[i] : _dataSlots[i];
        if (isCode)
        {
            nwramMapWramBSlot(slot,
                toDsp ? NWRAM_B_SLOT_MASTER_DSP_CODE : NWRAM_B_SLOT_MASTER_ARM9,
                toDsp ? i : slot, true);
        }
        else
        {
            nwramMapWramCSlot(slot,
                toDsp ? NWRAM_C_SLOT_MASTER_DSP_DATA : NWRAM_C_SLOT_MASTER_ARM9,
                toDsp ? i : slot, true);
        }
    }

    return true;
}

static bool Execute(void)
{
    dspPowerOn();
    dspSetCoreResetOff(0);
    dspSetSemaphoreMask(0);
    return true;
}

bool dspExecuteTLF(const void *tlf)
{
    const tlf_header *header = tlf;

    if (header->magic != TLF_MAGIC)
        return false;

    if (header->version != 0)
        return false;

    _slotB = 0xFF;
    _slotC = 0xFF;

    _codeSegs = 0xFF;
    _dataSegs = 0xFF;

    for (int i = 0; i < NWRAM_BC_SLOT_COUNT; i++)
    {
        _codeSlots[i] = i;
        _dataSlots[i] = i;

        nwramMapWramBSlot(i, NWRAM_B_SLOT_MASTER_ARM9, i, true);

        u32 *slot = (u32 *)(nwramGetBlockAddress(NWRAM_BLOCK_B)
                  + i * NWRAM_BC_SLOT_SIZE);

        for (int j = 0; j < (NWRAM_BC_SLOT_SIZE >> 2); j++)
            *slot++ = 0;

        nwramMapWramCSlot(i, NWRAM_C_SLOT_MASTER_ARM9, i, true);

        slot = (u32 *)(nwramGetBlockAddress(NWRAM_BLOCK_C)
             + i * NWRAM_BC_SLOT_SIZE);

        for (int j = 0; j < (NWRAM_BC_SLOT_SIZE >> 2); j++)
            *slot++ = 0;
    }

    for (int i = 0; i < header->num_sections; i++)
    {
        const tlf_section_header *section = &(header->section[i]);
        const void *data = ((const char *)tlf) + section->data_offset;
        bool isCode = section->type == TLF_SEGMENT_CODE;
        memcpy(DspToArm9Address(isCode, section->address), data, section->size);
    }

    SetMemoryMapping(true, 0,
            (NWRAM_BC_SLOT_SIZE * NWRAM_BC_SLOT_COUNT) >> 1, true);
    SetMemoryMapping(false, 0,
            (NWRAM_BC_SLOT_SIZE * NWRAM_BC_SLOT_COUNT) >> 1, true);

    return Execute();
}
