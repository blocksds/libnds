// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2025 Edoardo Lolletti (edo9300)

#include <stdbool.h>
#include <string.h>

#include <nds.h>

static u8 nand_ctr_iv[16] = {0};
static const u8 empty[16] = {0};

bool nandCrypt_Initialized(void)
{
    return memcmp(empty, nand_ctr_iv, 16) != 0;
}

void nandCrypt_Init(void)
{
    if (nandCrypt_Initialized())
        return;

    // "Activate" the key Y to generate the normal key
    ((volatile u32*)(AES_KEYSLOT3.key_y))[3] = 0xE1A00005;

    // Calculate the Input Vector used for NAND decryption
    // First 16 bytes of the SHA of the nand cid will be used
    // as base for the input vector
    u8 CID[16];
    SDMMC_getCidRaw(SDMMC_DEV_eMMC, (u32*)CID);
    u8 sha1Digest[20];
    swiSHA1Calc(sha1Digest, CID, 16);
    memcpy(nand_ctr_iv, sha1Digest, 16);
}

// add a 32bit int to a 128bit little endian value
static void u128_add32(const u8 *a, u32 b, vu8 *dest)
{
    u8 carry = 0;
    for (int i = 0; i < 16; i++)
    {
        u16 sum = a[i] + (b & 0xff) + carry;
        dest[i] = sum & 0xff;
        carry = sum >> 8;
        b >>= 8;
    }
}

void nandCrypt_SetIV(u32 offset)
{
    u128_add32(nand_ctr_iv, offset, REG_AES_IV);
}
