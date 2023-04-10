// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2023 Antonio Niño Díaz

#ifndef NDS_ARM9_CP15_ASM_H__
#define NDS_ARM9_CP15_ASM_H__

// Information from the "ARM 946E-S Technical Reference Manual" ARM DDI 0155A.
//
// NOTE: SBO means "should be one". They are bits that should always be set to
// one even // if they don't have a documented meaning.

#ifndef BIT
#define BIT(n)          (1 << (n))
#endif

#ifndef MASK
#define MASK(t, b)      ((BIT(t) - 1) - (BIT(b) - 1))
#endif

// General definitions for the NDS

#define ICACHE_SIZE             0x2000
#define DCACHE_SIZE             0x1000
#define CACHE_LINE_SIZE         32
#define ENTRIES_PER_SEGMENT     4

// Register 0, ID code register

#define CP15_REG0_ID_CODE_REG(rd)               p15, 0, rd, c0, c0, 0

#define CP15_ID_IMPLEMENTOR_MASK                MASK(31, 24) // 0x41
#define CP15_ID_ARCHITECTURE_VERSION_MASK       MASK(19, 16) // 0x4
#define CP15_ID_PART_NUMBER_MASK                MASK(15, 4) // 0x946
#define CP15_ID_VERSION_MASK                    MASK(3, 0)

// Register 0, Cache type register

#define CP15_REG0_CACHE_TYPE(rd)                p15, 0, rd, c0, c0, 1

#define CP15_CTYPE_CACHE_TYPE_MASK              MASK(28, 25)
#define CP15_CTYPE_HARVARD_UNIFIED              BIT(24)
#define CP15_CTYPE_DCACHE_SIZE_MASK             MASK(21, 18)
#define CP15_CTYPE_DCACHE_ASSOCIATIVITY_MASK    MASK(17, 15)
#define CP15_CTYPE_DCACHE_BASE_SIZE             BIT(14)
#define CP15_CTYPE_DCACHE_WORDS_PER_LINE_MASK   MASK(13, 12) // 0b10 = 8 words per line
#define CP15_CTYPE_ICACHE_SIZE_MASK             MASK(9, 6)
#define CP15_CTYPE_ICACHE_ASSOCIATIVITY_MASK    MASK(5, 3)
#define CP15_CTYPE_ICACHE_BASE_SIZE             BIT(2)
#define CP15_CTYPE_ICACHE_WORDS_PER_LINE_MASK   MASK(1, 0) // 0b10 = 8 words per line

// Register 0, Tightly-coupled memory size register

#define CP15_REG0_TCM_SIZE(rd)                      p15, 0, rd, c0, c0, 2

#define CP15_TCM_DATA_RAM_SIZE_MASK                 MASK(21, 18)
#define CP15_TCM_DATA_RAM_ABSENT                    BIT(14)
#define CP15_TCM_INSTRUCTION_RAM_SIZE_MASK          MASK(9, 6)
#define CP15_TCM_INSTRUCTION_RAM_ABSENT             BIT(2)

// Register 1, Control Register

#define CP15_REG1_CONTROL_REGISTER(rd)              p15, 0, rd, c1, c0, 0

#define CP15_CONTROL_ITCM_LOAD_MODE                 BIT(19)
#define CP15_CONTROL_ITCM_ENABLE                    BIT(18)
#define CP15_CONTROL_DTCM_LOAD_MODE                 BIT(17)
#define CP15_CONTROL_DTCM_ENABLE                    BIT(16)
#define CP15_CONTROL_DISABLE_LOADING_TBIT           BIT(15)
#define CP15_CONTROL_ROUND_ROBIN                    BIT(14)
#define CP15_CONTROL_ALTERNATE_VECTOR_SELECT        BIT(13)
#define CP15_CONTROL_ICACHE_ENABLE                  BIT(12)
#define CP15_CONTROL_BIG_ENDIAN                     BIT(7)
#define CP15_CONTROL_DCACHE_ENABLE                  BIT(2)
#define CP15_CONTROL_PROTECTION_UNIT_ENABLE         BIT(0)
#define CP15_CONTROL_RESERVED_SBO_MASK              0x78

// Register 2, Cache configuration registers

#define CP15_REG2_DATA_CACHE_CONFIG(rd)             p15, 0, rd, c2, c0, 0
#define CP15_REG2_INSTRUCTION_CACHE_CONFIG(rd)      p15, 0, rd, c2, c0, 1

#define CP15_CONFIG_AREA_IS_CACHABLE(n)             BIT(n) // 0 to 7

// Register 3, Write buffer control register

#define CP15_REG3_WRITE_BUFFER_CONTROL(rd)          p15, 0, rd, c3, c0, 0

#define CP15_CONFIG_AREA_IS_BUFFERABLE(n)           BIT(n) // 0 to 7

// Register 5, Access permission registers

#define CP15_REG5_DATA_ACCESS_PERMISSION(rd)        p15, 0, rd, c5, c0, 2
#define CP15_REG5_INSTRUCTION_ACCESS_PERMISSION(rd) p15, 0, rd, c5, c0, 3

#define CP15_ACCESS_PERMISSIONS_AREA_MASK(n)        (0xF << ((n) * 4))

#define CP15_AREA_ACCESS_PERMISSIONS_PNO_UNO(n)     (0x0 << ((n) * 4))
#define CP15_AREA_ACCESS_PERMISSIONS_PRW_UNO(n)     (0x1 << ((n) * 4))
#define CP15_AREA_ACCESS_PERMISSIONS_PRW_URO(n)     (0x2 << ((n) * 4))
#define CP15_AREA_ACCESS_PERMISSIONS_PRW_URW(n)     (0x3 << ((n) * 4))
#define CP15_AREA_ACCESS_PERMISSIONS_PRO_UNO(n)     (0x5 << ((n) * 4))
#define CP15_AREA_ACCESS_PERMISSIONS_PRO_URO(n)     (0x6 << ((n) * 4))

// Register 6, Protection region/base size registers

#define CP15_REG6_PROTECTION_REGION(rd, n)      p15, 0, rd, c6, c##n, 0

#define CP15_CONFIG_REGION_BASE_MASK            MASK(31, 12)
#define CP15_CONFIG_REGION_SIZE_MASK            MASK(5, 1)
#define CP15_CONFIG_REGION_ENABLE               BIT(0)

#define CP15_REGION_SIZE_4KB                    (0x0B << 1)
#define CP15_REGION_SIZE_8KB                    (0x0C << 1)
#define CP15_REGION_SIZE_16KB                   (0x0D << 1)
#define CP15_REGION_SIZE_32KB                   (0x0E << 1)
#define CP15_REGION_SIZE_64KB                   (0x0F << 1)
#define CP15_REGION_SIZE_128KB                  (0x10 << 1)
#define CP15_REGION_SIZE_256KB                  (0x11 << 1)
#define CP15_REGION_SIZE_512KB                  (0x12 << 1)
#define CP15_REGION_SIZE_1MB                    (0x13 << 1)
#define CP15_REGION_SIZE_2MB                    (0x14 << 1)
#define CP15_REGION_SIZE_4MB                    (0x15 << 1)
#define CP15_REGION_SIZE_8MB                    (0x16 << 1)
#define CP15_REGION_SIZE_16MB                   (0x17 << 1)
#define CP15_REGION_SIZE_32MB                   (0x18 << 1)
#define CP15_REGION_SIZE_64MB                   (0x19 << 1)
#define CP15_REGION_SIZE_128MB                  (0x1A << 1)
#define CP15_REGION_SIZE_256MB                  (0x1B << 1)
#define CP15_REGION_SIZE_512MB                  (0x1C << 1)
#define CP15_REGION_SIZE_1GB                    (0x1D << 1)
#define CP15_REGION_SIZE_2GB                    (0x1E << 1)
#define CP15_REGION_SIZE_4GB                    (0x1F << 1)

// Register 7, Cache operations register

#define CP15_REG7_FLUSH_ICACHE                          p15, 0, r0, c7, c5, 0
#define CP15_REG7_FLUSH_ICACHE_ENTRY(rd)                p15, 0, rd, c7, c5, 1
#define CP15_REG7_PREFETCH_ICACHE_LINE(rd)              p15, 0, rd, c7, c13, 1
#define CP15_REG7_FLUSH_DCACHE                          p15, 0, r0, c7, c6, 0
#define CP15_REG7_FLUSH_DCACHE_ENTRY(rd)                p15, 0, rd, c7, c6, 1
#define CP15_REG7_CLEAN_DCACHE_ENTRY(rd)                p15, 0, rd, c7, c10, 1
#define CP15_REG7_CLEAN_FLUSH_DCACHE_ENTRY(rd)          p15, 0, rd, c7, c14, 1
#define CP15_REG7_CLEAN_DCACHE_ENTRY_BY_INDEX(rd)       p15, 0, rd, c7, c10, 2
#define CP15_REG7_CLEAN_FLUSH_DCACHE_ENTRY_BY_INDEX(rd) p15, 0, rd, c7, c14, 2

#define CP15_REG7_DRAIN_WRITE_BUFFER                    p15, 0, r0, c7, c10, 4

#define CP15_REG7_WAIT_FOR_INTERRUPT                    p15, 0, r0, c7, c0, 4

// Register 9, Cache lockdown registers

#define CP15_REG9_DATA_LOCKDOWN_CONTROL(rd)             p15, 0, rd, c9, c0, 0
#define CP15_REG9_INSTRUCTION_LOCKDOWN_CONTROL(rd)      p15, 0, rd, c9, c0, 1

#define CP15_CACHE_LOCKDOWN_LOAD_BIT                    BIT(31)
#define CP15_CACHE_LOCKDOWN_SEGMENT_MASK                MASK(1, 0)

// Register 9, Tightly-coupled memory region registers

#define CP15_REG9_DTCM_CONTROL(rd)                      p15, 0, rd, c9, c1, 0
#define CP15_REG9_ITCM_CONTROL(rd)                      p15, 0, rd, c9, c1, 1
// Same definitions as "Register 6, Protection region/base size registers"

// Register 13, Trace process identifier register

#define CP15_REG13_PROCESS_ID(rd)                       p15, 0, rd, c13, c1, 1

// Register 15, RAM and TAG BIST test registers

#define CP15_REG15_TAG_BIST_CONTROL(rd)                 p15, 0, rd, c15, c0, 1
#define CP15_REG15_RAM_BIST_CONTROL(rd)                 p15, 1, rd, c15, c0, 1
#define CP15_REG15_CACHE_RAM_BIST_CONTROL(rd)           p15, 2, rd, c15, c0, 1

#define CP15_REG15_INSTR_TAG_BIST_ADDRESS(rd)           p15, 0, rd, c15, c0, 2
#define CP15_REG15_INSTR_TAG_BIST_GENERAL(rd)           p15, 0, rd, c15, c0, 3

#define CP15_REG15_DATA_TAG_BIST_ADDRESS(rd)            p15, 0, rd, c15, c0, 6
#define CP15_REG15_DATA_TAG_BIST_GENERAL(rd)            p15, 0, rd, c15, c0, 7

#define CP15_REG15_ITCM_TAG_BIST_ADDRESS(rd)            p15, 1, rd, c15, c0, 2
#define CP15_REG15_ITCM_TAG_BIST_GENERAL(rd)            p15, 1, rd, c15, c0, 3

#define CP15_REG15_DTCM_TAG_BIST_ADDRESS(rd)            p15, 1, rd, c15, c0, 6
#define CP15_REG15_DTCM_TAG_BIST_GENERAL(rd)            p15, 1, rd, c15, c0, 7

#define CP15_REG15_INSTR_CACHE_RAM_TAG_BIST_ADDRESS(rd) p15, 2, Rd, c15, c0, 2
#define CP15_REG15_INSTR_CACHE_RAM_TAG_BIST_GENERAL(rd) p15, 2, Rd, c15, c0, 3

#define CP15_REG15_DATA_CACHE_RAM_TAG_BIST_ADDRESS(rd)  p15, 2, Rd, c15, c0, 6
#define CP15_REG15_DATA_CACHE_RAM_TAG_BIST_GENERAL(rd)  p15, 2, Rd, c15, c0, 7

// Register 15, Test state register

#define CP15_REG15_TEST_STATE(rd)                   p15, 0, rd, c15, c0, 0

#define CP15_TEST_STATE_DISABLE_DCACHE_STREAMING    BIT(12)
#define CP15_TEST_STATE_DISABLE_ICACHE_STREAMING    BIT(11)
#define CP15_TEST_STATE_DISABLE_DCACHE_LINEFILL     BIT(10)
#define CP15_TEST_STATE_DISABLE_ICACHE_LINEFILL     BIT(9)

// Register 15, Cache debug index register

#define CP15_REG15_CACHE_DEBUG_BY_INDEX(rd)         p15, 3, rd, c15, c0, 0
#define CP15_REG15_INSTRUCTION_TAG(rd)              p15, 3, rd, c15, c1, 0
#define CP15_REG15_DATA_TAG(rd)                     p15, 3, rd, c15, c2, 0
#define CP15_REG15_INSTRUCTION_CACHE(rd)            p15, 3, rd, c15, c3, 0
#define CP15_REG15_DATA_CACHE(rd)                   p15, 3, rd, c15, c4, 0

#endif // NDS_ARM9_CP15_ASM_H__
