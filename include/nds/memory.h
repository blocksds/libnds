// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2005 Michael Noland (joat)
// Copyright (C) 2005 Jason Rogers (dovoto)

// Declaration of memory regions

/// @file nds/memory.h
///
/// @brief Defines for many of the regions of memory on the DS as well as a few
/// control functions for memory bus access.

#ifndef LIBNDS_NDS_MEMORY_H__
#define LIBNDS_NDS_MEMORY_H__

#include <assert.h>

#include <nds/ndstypes.h>

#ifdef ARM9
#define REG_EXMEMCNT    (*(vu16 *)0x04000204)
#else
#define REG_EXMEMSTAT   (*(vu16 *)0x04000204)
#endif

#define EXMEMCNT_SRAM_TIME_10_CYCLES        (0)
#define EXMEMCNT_SRAM_TIME_8_CYCLES         (1)
#define EXMEMCNT_SRAM_TIME_6_CYCLES         (2)
#define EXMEMCNT_SRAM_TIME_18_CYCLES        (3)
#define EXMEMCNT_SRAM_TIME_MASK             (3)
#define EXMEMCNT_ROM_TIME1_10_CYCLES        (0)
#define EXMEMCNT_ROM_TIME1_8_CYCLES         (1 << 2)
#define EXMEMCNT_ROM_TIME1_6_CYCLES         (2 << 2)
#define EXMEMCNT_ROM_TIME1_18_CYCLES        (3 << 2)
#define EXMEMCNT_ROM_TIME1_MASK             (3 << 2)
#define EXMEMCNT_ROM_TIME2_6_CYCLES         (0)
#define EXMEMCNT_ROM_TIME2_4_CYCLES         (1 << 4)
#define EXMEMCNT_ROM_TIME2_MASK             (1 << 4)
#define EXMEMCNT_PHI_CLOCK_OFF              (0)
#define EXMEMCNT_PHI_CLOCK_4MHZ             (1 << 5)
#define EXMEMCNT_PHI_CLOCK_8MHZ             (2 << 5)
#define EXMEMCNT_PHI_CLOCK_16MHZ            (3 << 5)
#define EXMEMCNT_PHI_CLOCK_MASK             (3 << 5)
#define EXMEMCNT_CART_ARM7                  BIT(7)
#define EXMEMCNT_CARD_ARM7                  BIT(11)
#define EXMEMCNT_MAIN_RAM_PRIORITY_ARM7     BIT(15)

#define ARM7_MAIN_RAM_PRIORITY              EXMEMCNT_MAIN_RAM_PRIORITY_ARM7
#define ARM7_OWNS_CARD                      EXMEMCNT_CARD_ARM7
#define ARM7_OWNS_ROM                       EXMEMCNT_CART_ARM7

#define REG_MBK1                    ((vu8 *)0x04004040) // WRAM_A 0..3
#define REG_MBK2                    ((vu8 *)0x04004044) // WRAM_B 0..3
#define REG_MBK3                    ((vu8 *)0x04004048) // WRAM_B 4..7
#define REG_MBK4                    ((vu8 *)0x0400404C) // WRAM_C 0..3
#define REG_MBK5                    ((vu8 *)0x04004050) // WRAM_C 4..7
#define REG_MBK6                    (*(vu32 *)0x04004054)

#define MBK6_START_ADDR_MASK        0x00000FF0
#define MBK6_START_ADDR_SHIFT       4
#define MBK6_IMAGE_SIZE_SHIFT       12
#define MBK6_END_ADDR_SHIFT         20

#define REG_MBK7                    (*(vu32 *)0x04004058)

#define MBK7_START_ADDR_MASK        0x00000FF8
#define MBK7_START_ADDR_SHIFT       3
#define MBK7_IMAGE_SIZE_SHIFT       12
#define MBK7_END_ADDR_SHIFT         19

#define REG_MBK8                    (*(vu32 *)0x0400405C)

#define MBK8_START_ADDR_MASK        0x00000FF8
#define MBK8_START_ADDR_SHIFT       3
#define MBK8_IMAGE_SIZE_SHIFT       12
#define MBK8_END_ADDR_SHIFT         19

#define REG_MBK9                    (*(vu32 *)0x04004060)

// Protection register (write-once sadly)
#ifdef ARM7
#define PROTECTION  (*(vu32 *)0x04000308)
#endif

/// 8 bit pointer to the start of all the RAM.
#define ALLRAM      ((u8*)0x00000000)

/// 8 bit pointer to main RAM.
#define MAINRAM8    ((u8 *)0x02000000)
/// 16 bit pointer to main RAM.
#define MAINRAM16   ((u16 *)0x02000000)
/// 32 bit pointer to main RAM.
#define MAINRAM32   ((u32 *)0x02000000)

// TODO: Fix shared RAM

// GBA_BUS is volatile, while GBAROM is not

/// 16 bit volatile pointer to the GBA slot bus.
#define GBA_BUS     ((vu16 *)(0x08000000))
/// 16 bit pointer to the GBA slot ROM.
#define GBAROM      ((u16 *)0x08000000)

/// 8 bit pointer to GBA slot Save RAM.
#define SRAM        ((u8 *)0x0A000000)

#ifdef ARM7
#define VRAM        ((u16 *)0x06000000)
#endif

/// GBA file header format.
///
/// See gbatek for more info.
typedef struct sGBAHeader {
    u32 entryPoint;   ///< 32 bits ARM opcode to jump to executable code
    u8 logo[156];     ///< Nintendo logo needed for booting the game
    char title[12];   ///< Game title
    char gamecode[4]; ///< Game code
    u16 makercode;    ///< Identifies the (commercial) developer
    u8 is96h;         ///< Fixed value that is always 96h
    u8 unitcode;      ///< Identifies the required hardware
    u8 devicecode;    ///< Used by Nintedo's hardware debuggers. Normally 0
    u8 unused[7];
    u8 version;       ///< The version of the game.
    u8 complement;    ///< Complement checksum of the gba header
    u16 checksum;     ///< A 16 bit checksum? (gbatek says its unused/reserved)
} tGBAHeader;

#define GBA_HEADER  (*(tGBAHeader *)0x08000000)

/// NDS file header format
///
/// See gbatek for more info.
typedef struct sNDSHeader {
    char gameTitle[12];       ///< 12 characters for the game title.
    char gameCode[4];         ///< 4 characters for the game code.
    char makercode[2];        ///< Identifies the (commercial) developer.
    u8 unitCode;              ///< Identifies the required hardware.
    u8 deviceType;            ///< Type of device in the game card
    u8 deviceSize;            ///< Capacity of the device (1 << n Mbit)
    u8 reserved1[9];
    u8 romversion;            ///< Version of the ROM.
    u8 flags;                 ///< Bit 2: auto-boot flag.

    u32 arm9romOffset;        ///< Offset of the ARM9 binary in the nds file.
    void *arm9executeAddress; ///< Adress that should be executed after the binary has been copied.
    void *arm9destination;    ///< Destination address to where the ARM9 binary should be copied.
    u32 arm9binarySize;       ///< Size of the ARM9 binary.

    u32 arm7romOffset;        ///< Offset of the ARM7 binary in the nds file.
    void *arm7executeAddress; ///< Adress that should be executed after the binary has been copied.
    void *arm7destination;    ///< Destination address to where the ARM7 binary should be copied.
    u32 arm7binarySize;       ///< Size of the ARM7 binary.

    u32 filenameOffset;       ///< File Name Table (FNT) offset.
    u32 filenameSize;         ///< File Name Table (FNT) size.
    u32 fatOffset;            ///< File Allocation Table (FAT) offset.
    u32 fatSize;              ///< File Allocation Table (FAT) size.

    u32 arm9overlaySource;    ///< File ARM9 overlay offset.
    u32 arm9overlaySize;      ///< File ARM9 overlay size.
    u32 arm7overlaySource;    ///< File ARM7 overlay offset.
    u32 arm7overlaySize;      ///< File ARM7 overlay size.

    u32 cardControl13;        ///< Port 40001A4h setting for normal commands (used in modes 1 and 3)
    u32 cardControlBF;        ///< Port 40001A4h setting for KEY1 commands (used in mode 2)
    u32 bannerOffset;         ///< Offset to the banner with icon and titles etc.

    u16 secureCRC16;          ///< Secure Area Checksum, CRC-16.

    u16 readTimeout;          ///< Secure Area Loading Timeout.

    u32 unknownRAM1;          ///< ARM9 Auto Load List RAM Address (?)
    u32 unknownRAM2;          ///< ARM7 Auto Load List RAM Address (?)

    u32 bfPrime1;             ///< Secure Area Disable part 1.
    u32 bfPrime2;             ///< Secure Area Disable part 2.
    u32 romSize;              ///< total size of the ROM.

    u32 headerSize;           ///< ROM header size.
    u32 zeros88[14];
    u8 gbaLogo[156];          ///< Nintendo logo needed for booting the game.
    u16 logoCRC16;            ///< Nintendo Logo Checksum, CRC-16.
    u16 headerCRC16;          ///< Header checksum, CRC-16.

} tNDSHeader;

typedef struct __DSiHeader {
    tNDSHeader ndshdr;
    u32 debugRomSource;       ///< Debug ROM offset.
    u32 debugRomSize;         ///< Debug size.
    u32 debugRomDestination;  ///< Debug RAM destination.
    u32 offset_0x16C;         // Reserved?

    u8 zero[0x10];

    u8 global_mbk_setting[5][4];
    u32 arm9_mbk_setting[3];
    u32 arm7_mbk_setting[3];
    u32 mbk9_wramcnt_setting;

    u32 region_flags;
    u32 access_control;
    u32 scfg_ext_mask;
    u8 offset_0x1BC[3];
    u8 appflags;

    void *arm9iromOffset;
    u32 offset_0x1C4;
    void *arm9idestination;
    u32 arm9ibinarySize;
    void *arm7iromOffset;
    u32 offset_0x1D4;
    void *arm7idestination;
    u32 arm7ibinarySize;

    u32 digest_ntr_start;
    u32 digest_ntr_size;
    u32 digest_twl_start;
    u32 digest_twl_size;
    u32 sector_hashtable_start;
    u32 sector_hashtable_size;
    u32 block_hashtable_start;
    u32 block_hashtable_size;
    u32 digest_sector_size;
    u32 digest_block_sectorcount;

    u32 banner_size;
    u32 offset_0x20C;
    u32 total_rom_size;
    u32 offset_0x214;
    u32 offset_0x218;
    u32 offset_0x21C;

    u32 modcrypt1_start;
    u32 modcrypt1_size;
    u32 modcrypt2_start;
    u32 modcrypt2_size;

    u32 tid_low;
    u32 tid_high;
    u32 public_sav_size;
    u32 private_sav_size;
    u8 reserved3[176];
    u8 age_ratings[0x10];

    u8 hmac_arm9[20];
    u8 hmac_arm7[20];
    u8 hmac_digest_master[20];
    u8 hmac_icon_title[20];
    u8 hmac_arm9i[20];
    u8 hmac_arm7i[20];
    u8 reserved4[40];
    u8 hmac_arm9_no_secure[20];
    u8 reserved5[2636];
    u8 debug_args[0x180];
    u8 rsa_signature[0x80];

} tDSiHeader;


#define __NDSHeader ((tNDSHeader *)0x02FFFE00)
#define __DSiHeader ((tDSiHeader *)0x02FFE000)

/// NDS banner format.
///
/// See gbatek for more information.
typedef struct sNDSBanner {
  u16 version;        ///< Version of the banner.
  u16 crc;            ///< 16 bit crc/checksum of the banner.
  u8 reserved[28];
  u8 icon[512];       ///< 32 * 32 icon of the game with 4 bit per pixel.
  u16 palette[16];    ///< The pallete of the icon.
  u16 titles[6][128]; ///< Title of the game in 6 different languages.
} tNDSBanner;

#ifdef __cplusplus
extern "C" {
#endif

#ifdef ARM9
#define BUS_OWNER_ARM9 true
#define BUS_OWNER_ARM7 false

/// Sets the owner of the GBA cart.
///
/// Both CPUs cannot have access to the GBA cart at the same time (slot 2).
///
/// @param arm9 If true the ARM9 is the owner, otherwise the ARM7
static inline void sysSetCartOwner(bool arm9)
{
    REG_EXMEMCNT = (REG_EXMEMCNT & ~ARM7_OWNS_ROM) | (arm9 ? 0 : ARM7_OWNS_ROM);
}

/// Sets the owner of the DS card bus.
///
/// Both CPUs cannot have access to the DS card bus at the same time (slot 1).
///
/// @param arm9 If true the ARM9 is the owner, otherwise the ARM7
static inline void sysSetCardOwner(bool arm9)
{
    REG_EXMEMCNT = (REG_EXMEMCNT & ~ARM7_OWNS_CARD) | (arm9 ? 0 : ARM7_OWNS_CARD);
}

/// Sets the owner of the DS card bus (slot 1) and GBA cart bus (slot 2).
///
/// Only one CPU may access the devices at a time.
///
/// @param arm9rom If true the ARM9 is the owner of slot 2, otherwise the ARM7
/// @param arm9card If true the ARM9 is the owner of slot 1, otherwise the ARM7
static inline void sysSetBusOwners(bool arm9rom, bool arm9card)
{
    REG_EXMEMCNT = (REG_EXMEMCNT & ~(ARM7_OWNS_CARD | ARM7_OWNS_ROM))
                    | (arm9card ? 0 : ARM7_OWNS_CARD)
                    | (arm9rom ? 0 : ARM7_OWNS_ROM);
}
#endif // ARM9

#ifdef __cplusplus
}
#endif

#endif // LIBNDS_NDS_MEMORY_H__
