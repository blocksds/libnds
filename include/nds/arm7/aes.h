// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2017 Dave Murphy (WinterMute)

// ARM7 AES

#ifndef LIBNDS_NDS_ARM7_AES_H__
#define LIBNDS_NDS_ARM7_AES_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <nds/ndstypes.h>

typedef struct aes_keyslot {
    vu8 normalkey[16];
    vu8 key_x[16];
    vu8 key_y[16];
} aes_keyslot_t;

#define REG_AES_CNT             (*(vu32 *)0x04004400)

#define AES_WRFIFO_FLUSH        (1 << 10)
#define AES_RDFIFO_FLUSH        (1 << 11)

#define AES_CNT_DMA_WRITE_SIZE(size) (((size) & 3) << 12)
#define AES_CNT_DMA_READ_SIZE(size)  (((size) & 3) << 14)

#define AES_CNT_CCM_SIZE(size)       (((size) & 3) << 16)

#define AES_CCM_PASSTRHOUGH     (1 << 19)

#define AES_CNT_KEY_APPLY       (1 << 24)

#define AES_CNT_KEYSLOT(slot)   (((slot) & 3) << 26)

#define AES_CNT_MODE(mode)      (((mode) & 3) << 28)

#define AES_CNT_IRQ             (1 << 30)

#define AES_CNT_ENABLE          (1 << 31)

#define REG_AES_BLKCNT  (*(vu32 *)0x4004404)

#define REG_AES_WRFIFO  (*(vu32 *)0x4004408)
#define REG_AES_RDFIFO  (*(vu32 *)0x400440c)

#define REG_AES_IV      ((vu8 *)0x4004420)
#define REG_AES_MAC     ((vu8 *)0x4004430)

#define AES_KEYSLOT     ((aes_keyslot_t *)0x4004440)
#define AES_KEYSLOT0    (*(aes_keyslot_t *)0x4004440)
#define AES_KEYSLOT1    (*(aes_keyslot_t *)0x4004470)
#define AES_KEYSLOT2    (*(aes_keyslot_t *)0x40044A0)
#define AES_KEYSLOT3    (*(aes_keyslot_t *)0x40044D0)

#ifdef __cplusplus
}
#endif

#endif // LIBNDS_NDS_ARM7_AES_H__
