// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2020 Gericom
// Copyright (c) 2023 Antonio Niño Díaz

#include <stdint.h>

#include <nds/nwram.h>
#include <nds/memory.h>

u32 nwramGetBlockAddress(NWRAM_BLOCK block)
{
    uint32_t start;

    switch (block)
    {
        case NWRAM_BLOCK_A:
            start = (REG_MBK6 & MBK6_START_ADDR_MASK) >> MBK6_START_ADDR_SHIFT;
            return NWRAM_BASE + (start << NWRAM_A_SLOT_SHIFT);

        case NWRAM_BLOCK_B:
            start = (REG_MBK7 & MBK7_START_ADDR_MASK) >> MBK7_START_ADDR_SHIFT;
            return NWRAM_BASE + (start << NWRAM_BC_SLOT_SHIFT);

        case NWRAM_BLOCK_C:
            start = (REG_MBK8 & MBK8_START_ADDR_MASK) >> MBK8_START_ADDR_SHIFT;
            return NWRAM_BASE + (start << NWRAM_BC_SLOT_SHIFT);
    }

    return 0;
}

void nwramSetBlockMapping(NWRAM_BLOCK block, u32 start, u32 length,
                          NWRAM_BLOCK_IMAGE_SIZE imageSize)
{
    start -= NWRAM_BASE;
    u32 end;
    switch (block)
    {
        case NWRAM_BLOCK_A:
            start >>= NWRAM_A_SLOT_SHIFT;
            length >>= NWRAM_A_SLOT_SHIFT;
            end = start + length;
            REG_MBK6 = (start << MBK6_START_ADDR_SHIFT)
                     | (imageSize << MBK6_IMAGE_SIZE_SHIFT)
                     | (end << MBK6_END_ADDR_SHIFT);
            break;

        case NWRAM_BLOCK_B:
            start >>= NWRAM_BC_SLOT_SHIFT;
            length >>= NWRAM_BC_SLOT_SHIFT;
            end = start + length;
            REG_MBK7 = (start << MBK7_START_ADDR_SHIFT)
                     | (imageSize << MBK7_IMAGE_SIZE_SHIFT)
                     | (end << MBK7_END_ADDR_SHIFT);
            break;

        case NWRAM_BLOCK_C:
            start >>= NWRAM_BC_SLOT_SHIFT;
            length >>= NWRAM_BC_SLOT_SHIFT;
            end = start + length;
            REG_MBK8 = (start << MBK8_START_ADDR_SHIFT)
                     | (imageSize << MBK8_IMAGE_SIZE_SHIFT)
                     | (end << MBK8_END_ADDR_SHIFT);
            break;
    }
}

#ifdef ARM9
void nwramMapWramASlot(int slot, NWRAM_A_SLOT_MASTER master, int offset, bool enable)
{
    if (slot < 0 || slot > 3 || offset < 0 || offset > 3)
        return;

    REG_MBK1[slot] = enable ?
        (NWRAM_A_SLOT_ENABLE | master | NWRAM_A_SLOT_OFFSET(offset)) : 0;
}

void nwramMapWramBSlot(int slot, NWRAM_B_SLOT_MASTER master, int offset, bool enable)
{
    if (slot < 0 || slot > 7 || offset < 0 || offset > 7)
        return;

    REG_MBK2[slot] = enable ?
        (NWRAM_BC_SLOT_ENABLE | master | NWRAM_BC_SLOT_OFFSET(offset)) : 0;
}

void nwramMapWramCSlot(int slot, NWRAM_C_SLOT_MASTER master, int offset, bool enable)
{
    if (slot < 0 || slot > 7 || offset < 0 || offset > 7)
        return;

    REG_MBK4[slot] = enable ?
        (NWRAM_BC_SLOT_ENABLE | master | NWRAM_BC_SLOT_OFFSET(offset)) : 0;
}
#endif // ARM9
