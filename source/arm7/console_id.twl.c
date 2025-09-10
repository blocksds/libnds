// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2025 Edoardo Lolletti (edo9300)

#include <string.h>

#include <nds.h>

static void computeAes(void *out)
{
    REG_AES_BLKCNT = 1 << 16;
    REG_AES_CNT = AES_CNT_MODE(2) |
                  AES_WRFIFO_FLUSH |
                  AES_RDFIFO_FLUSH |
                  AES_CNT_KEY_APPLY |
                  AES_CNT_KEYSLOT(3) |
                  AES_CNT_ENABLE;

    u32 *out32 = (u32*)out;

    for (int i = 0; i < 4; ++i)
        REG_AES_WRFIFO = 0;

    while (((REG_AES_CNT >> 0x5) & 0x1F) < 0x4);

    for (int i = 0; i < 4; ++i)
        out32[i] = REG_AES_RDFIFO;
}

// In most of the contexes, the aes keyslot 3 is configured with the keys required
// to decrypt the console nand
// The key X from this keyslot, is populated with the console id at bytes 0-3 and 12-15
static void computeConsoleIdFromNandKeyX(aes_keyslot_t *keyslot, u8 ConsoleIdOut[8])
{
    // "enable" the keyslot 3 for nand crypto, so that the keys are properly derived
    ((vu32*)(AES_KEYSLOT3.key_y))[3] = 0xE1A00005;
    u8 canary[16] = { 0 };
    computeAes(canary);

    u8 key_y_oracle = 0xE1;

    for (int i = 0; i < 8; i++)
    {
        for (int j = 0; j < 256; j++)
        {
            u8 scratch[0x10];
            int key_x_idx = i;
            if (i >= 4)
                key_x_idx += 8;
            keyslot->key_x[key_x_idx] = j & 0xFF;
            keyslot->key_y[15] = key_y_oracle;
            computeAes(scratch);
            if (!memcmp(scratch, canary, 16))
            {
                ConsoleIdOut[i] = j;
                break;
            }
        }
    }
}

u64 getConsoleID(void)
{
    // first check whether we can read the console ID directly and it was not hidden by SCFG
    if ((REG_SCFG_ROM & (1u << 10)) == 0 && (REG_CONSOLEID_FLAG & 0x1) == 0)
    {
        return REG_CONSOLEID;
    }

    u64 consoleId;
    computeConsoleIdFromNandKeyX(&AES_KEYSLOT3, (u8*)&consoleId);
    return consoleId;
}
