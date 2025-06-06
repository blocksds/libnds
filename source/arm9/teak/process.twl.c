// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2020 Gericom
// Copyright (C) 2023 Antonio Niño Díaz

#include <assert.h>
#include <string.h>

#include <nds/arm9/teak/dsp.h>
#include <nds/arm9/teak/tlf.h>
#include <nds/memory.h>
#include <nds/nwram.h>
#include <nds/system.h>

static u16 _slotB;
static u16 _slotC;
static int _codeSegs;
static int _dataSegs;
static u8 _codeSlots[NWRAM_BC_SLOT_COUNT];
static u8 _dataSlots[NWRAM_BC_SLOT_COUNT];

static void *dspToArm9Address(bool isCodePtr, u32 addr)
{
    addr = DSP_MEM_ADDR_TO_CPU(addr);
    int seg = addr >> NWRAM_BC_SLOT_SHIFT;
    int offs = addr - (seg << NWRAM_BC_SLOT_SHIFT);
    int slot = isCodePtr ? _codeSlots[seg] : _dataSlots[seg];
    return (char *)nwramGetBlockAddress(isCodePtr ? NWRAM_BLOCK_B : NWRAM_BLOCK_C)
            + slot * NWRAM_BC_SLOT_SIZE + offs;
}

static int dspSetMemoryMapping(bool isCode, u32 addr, u32 len, bool toDsp)
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

        int ret;

        int slot = isCode ? _codeSlots[i] : _dataSlots[i];
        if (isCode)
        {
            ret = nwramMapWramBSlot(slot,
                toDsp ? NWRAM_B_SLOT_MASTER_DSP_CODE : NWRAM_B_SLOT_MASTER_ARM9,
                toDsp ? i : slot, true);
        }
        else
        {
            ret = nwramMapWramCSlot(slot,
                toDsp ? NWRAM_C_SLOT_MASTER_DSP_DATA : NWRAM_C_SLOT_MASTER_ARM9,
                toDsp ? i : slot, true);
        }

        if (ret < 0)
            return - 1;
    }

    return 0;
}

DSPExecResult dspExecuteTLF(const void *tlf)
{
    if (!nwramIsAvailable())
        return DSP_NOT_AVAILABLE;

    const tlf_header *header = tlf;

    if (header->magic != TLF_MAGIC)
        return DSP_TLF_BAD_MAGIC;

    if (header->version != 0)
        return DSP_TLF_BAD_VERSION;

    // Power DSP off before making any changes
    dspPowerOff();

    _slotB = 0xFF;
    _slotC = 0xFF;

    _codeSegs = 0xFF;
    _dataSegs = 0xFF;

    // Zero all memory mapped to the DSP
    for (int i = 0; i < NWRAM_BC_SLOT_COUNT; i++)
    {
        _codeSlots[i] = i;
        _dataSlots[i] = i;

        if (nwramMapWramBSlot(i, NWRAM_B_SLOT_MASTER_ARM9, i, true) < 0)
            return DSP_NOT_AVAILABLE;

        u32 *slot = (u32 *)(nwramGetBlockAddress(NWRAM_BLOCK_B)
                  + i * NWRAM_BC_SLOT_SIZE);

        for (int j = 0; j < (NWRAM_BC_SLOT_SIZE >> 2); j++)
            *slot++ = 0;

        if (nwramMapWramCSlot(i, NWRAM_C_SLOT_MASTER_ARM9, i, true) < 0)
            return DSP_NOT_AVAILABLE;

        slot = (u32 *)(nwramGetBlockAddress(NWRAM_BLOCK_C)
             + i * NWRAM_BC_SLOT_SIZE);

        for (int j = 0; j < (NWRAM_BC_SLOT_SIZE >> 2); j++)
            *slot++ = 0;
    }

    // Copy code and data sections to previously zeroed memory
    for (int i = 0; i < header->num_sections; i++)
    {
        const tlf_section_header *section = &(header->section[i]);
        const void *data = ((const char *)tlf) + section->data_offset;
        bool isCode = section->type == TLF_SEGMENT_CODE;
        memcpy(dspToArm9Address(isCode, section->address), data, section->size);
    }

    if (dspSetMemoryMapping(true, 0,
            (NWRAM_BC_SLOT_SIZE * NWRAM_BC_SLOT_COUNT) >> 1, true) != 0)
        return DSP_NOT_AVAILABLE;

    if (dspSetMemoryMapping(false, 0,
            (NWRAM_BC_SLOT_SIZE * NWRAM_BC_SLOT_COUNT) >> 1, true) != 0)
        return DSP_NOT_AVAILABLE;

    // Boot the DSP
    dspPowerOn();
    dspSetCoreResetOff(0);
    dspSetSemaphoreMask(0);

    return DSP_EXEC_OK;
}

DSPExecResult dspExecuteDefaultTLF(const void *tlf)
{
    if (!nwramIsAvailable())
        return DSP_NOT_AVAILABLE;

    // Power DSP off before making any changes
    dspPowerOff();

    // Map NWRAM to copy the DSP code
    nwramSetBlockMapping(NWRAM_BLOCK_B, 0x03000000, 256 * 1024,
                         NWRAM_BLOCK_IMAGE_SIZE_256K);
    nwramSetBlockMapping(NWRAM_BLOCK_C, 0x03400000, 256 * 1024,
                         NWRAM_BLOCK_IMAGE_SIZE_256K);

    int ret = dspExecuteTLF(tlf);

    // Remove NWRAM from the memory map
    nwramSetBlockMapping(NWRAM_BLOCK_B, NWRAM_BASE, 0, NWRAM_BLOCK_IMAGE_SIZE_32K);
    nwramSetBlockMapping(NWRAM_BLOCK_C, NWRAM_BASE, 0, NWRAM_BLOCK_IMAGE_SIZE_32K);

    return ret;
}
