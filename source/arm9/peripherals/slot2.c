// SPDX-License-Identifier: Zlib
//
// Copyright (c) 2023 Adrian "asie" Siekierka

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <nds/ndstypes.h>
#include <nds/memory.h>
#include <nds/system.h>
#include <nds/arm9/peripherals/slot2.h>

// Types

#define SLOT2_PERIPHERAL_LOCK 0x8000
#define SLOT2_EXMEMCNT_4_2 (EXMEMCNT_ROM_TIME1_10_CYCLES | EXMEMCNT_ROM_TIME2_6_CYCLES | EXMEMCNT_SRAM_TIME_18_CYCLES)
#define SLOT2_EXMEMCNT_3_1 (EXMEMCNT_ROM_TIME1_8_CYCLES | EXMEMCNT_ROM_TIME2_4_CYCLES | EXMEMCNT_SRAM_TIME_18_CYCLES)
#define SLOT2_EXMEMCNT_2_1 (EXMEMCNT_ROM_TIME1_6_CYCLES | EXMEMCNT_ROM_TIME2_4_CYCLES | EXMEMCNT_SRAM_TIME_18_CYCLES)

typedef struct {
    uint32_t gamecode;
    uint16_t peripheral_mask;
    uint8_t exmemcnt;
    uint8_t flags;
    // Detection process:
    // - call unlock with peripheral_mask
    // - call detect
    // - call lock
    bool (*detect)(void);
    void (*unlock)(uint32_t);
} slot2_definition_t;

#define SLOT2_DEVICE_NONE 0xFFFF

// State

uint16_t *slot2_extram_start;
uint32_t slot2_extram_size = 0;
uint16_t slot2_device_id = SLOT2_DEVICE_NONE;
uint16_t slot2_extram_banks = 0;

// Unlocking/locking/detection functions

#define EZ_CMD_SET_PSRAM_PAGE 0x9860000
#define EZ_CMD_SET_ROM_PAGE   0x9880000
#define EZ_CMD_SET_NOR_WRITE  0x9C40000

void slot2EzCommand(uint32_t address, uint16_t value) {
    *((vu16*)0x9FE0000) = 0xD200;
    *((vu16*)0x8000000) = 0x1500;
    *((vu16*)0x8020000) = 0xD200;
    *((vu16*)0x8040000) = 0x1500;
    *((vu16*)address) = value;
    *((vu16*)0x9FC0000) = 0x1500;
}

static void none_unlock(uint32_t unused) {
    (void)unused;

    // No unlocking/locking done.
}

static bool none_detect(void) {
    // The game code check was sufficient.
    return true;
}

static bool __extram_detect(uint32_t max_banks) {
    slot2_extram_size = 0;
    slot2_extram_banks = 0;

    uint32_t previous_size = 65536;
    uint32_t proposed_size = 131072;
    bool searching = true;
    while ((((uintptr_t) slot2_extram_start) + proposed_size) <= 0xA000000) {
        // ptr1 = old size final memory cell
        vu16 *ptr1 = ((vu16*) (((uintptr_t) slot2_extram_start) + previous_size - 2));
        // ptr2 = new size final memory cell
        vu16 *ptr2 = ((vu16*) (((uintptr_t) slot2_extram_start) + proposed_size - 2));
        u16 ptr2v = *ptr2;
        // Check if ptr2 can be written to
        *ptr2 ^= 0xFFFF;
        if ((*ptr2 ^ 0xFFFF) != ptr2v)
            searching = false;
        // Check if ptr2 mirrors ptr1
        if (*ptr1 != 0xAAAA) {
            *ptr2 = 0xAAAA;
            if (*ptr1 == 0xAAAA)
                searching = false;
        }
        if (*ptr1 != 0x5555) {
            *ptr2 = 0x5555;
            if (*ptr1 == 0x5555)
                searching = false;
        }
        // Restore ptr2 value
        *ptr2 = ptr2v;
        // Check if end of memory found
        if (!searching) break;
        // Update size
        slot2_extram_size = proposed_size;
        previous_size = proposed_size;
        // 128KB, 256KB, 512KB, 1MB, 1.5MB, 2MB, 2.5MB, ...
        if (proposed_size >= 524288)
            proposed_size += 524288;
        else
            proposed_size <<= 1;
    }
    if (!slot2_extram_size)
        return false;
    slot2_extram_banks = 1;
    if (max_banks > 1) {
        slot2EzCommand(EZ_CMD_SET_PSRAM_PAGE, 0);
        // Currently, only the EZO/EZODE support more than one bank.
        // This will need to be refactored if that changes.
        while (slot2_extram_banks < max_banks) {
            u16 old_value_bank0 = *slot2_extram_start;
            *slot2_extram_start = 0x0000;
            slot2EzCommand(EZ_CMD_SET_PSRAM_PAGE, slot2_extram_banks << 12);
            u16 old_value_bank1 = *slot2_extram_start;
            *slot2_extram_start = 0xFFFF;
            slot2EzCommand(EZ_CMD_SET_PSRAM_PAGE, 0);
            bool result = *slot2_extram_start == 0x0000;
            *slot2_extram_start = old_value_bank0;
            slot2EzCommand(EZ_CMD_SET_PSRAM_PAGE, slot2_extram_banks << 12);
            *slot2_extram_start = old_value_bank1;
            slot2EzCommand(EZ_CMD_SET_PSRAM_PAGE, 0);
            if (result)
                slot2_extram_banks++;
            else
                break;
        }
    }
    return true;
}

static bool extram_detect(void) {
    return __extram_detect(1);
}

static bool pak_rumble_detect(void) {
    for (int i = 0; i < 0x80; i++) {
        if (GBA_BUS[i] & BIT(1))
            return false;
    }
    return true;
}

// SuperCard

#define SC_REG_ENABLE       (*(vu16 *)0x9FFFFFE)
#define SC_ENABLE_MAGIC     0xA55A

#define SC_ENABLE_RAM       (1 << 0)
#define SC_ENABLE_CARD      (1 << 1)
#define SC_ENABLE_WRITE     (1 << 2) // To be used with SC_ENABLE_RAM
#define SC_ENABLE_RUMBLE    (1 << 3)

#define SC_ENABLE_FIRMWARE  (0)

static void supercard_unlock(uint32_t type) {
    SC_REG_ENABLE = SC_ENABLE_MAGIC;
    SC_REG_ENABLE = SC_ENABLE_MAGIC;

    uint32_t mode;
    if (type & SLOT2_PERIPHERAL_LOCK)
        mode = SC_ENABLE_FIRMWARE;
    else if (type & SLOT2_PERIPHERAL_RUMBLE_ANY)
        mode = SC_ENABLE_RUMBLE;
    else
        mode = SC_ENABLE_RAM | SC_ENABLE_WRITE;

    SC_REG_ENABLE = mode;
    SC_REG_ENABLE = mode;
}

static bool supercard_detect(void) {
    supercard_unlock(SLOT2_PERIPHERAL_EXTRAM);
    if (extram_detect()) return true;
    supercard_unlock(SLOT2_PERIPHERAL_RUMBLE_ANY);
    if (pak_rumble_detect()) return true;
    return false;
}

// M3

static void m3_unlock(uint32_t type) {
    *((vu16*) 0x08E00002);
    *((vu16*) 0x0800000E);
    *((vu16*) 0x08801FFC);
    *((vu16*) 0x0800104A);
    *((vu16*) 0x08800612);
    *((vu16*) 0x08000000);
    *((vu16*) 0x08801B66);
    *((vu16*) 0x08000000 + ((type & SLOT2_PERIPHERAL_LOCK ? 0x400003 : 0x400006) << 1));
    *((vu16*) 0x0800080E);
    *((vu16*) 0x08000000);
    *((vu16*) 0x080001E4);
    *((vu16*) 0x080001E4);
    *((vu16*) 0x08000188);
    *((vu16*) 0x08000188);
}

// G6

static void g6_unlock(uint32_t type) {
    *((vu16*) 0x09000000);
    *((vu16*) 0x09FFFFE0);
    *((vu16*) 0x09FFFFEC);
    *((vu16*) 0x09FFFFEC);
    *((vu16*) 0x09FFFFEC);
    *((vu16*) 0x09FFFFFC);
    *((vu16*) 0x09FFFFFC);
    *((vu16*) 0x09FFFFFC);
    *((vu16*) 0x09FFFF4A);
    *((vu16*) 0x09FFFF4A);
    *((vu16*) 0x09FFFF4A);
    *((vu16*) 0x09200000 + ((type & SLOT2_PERIPHERAL_LOCK ? 0x3 : 0x6) << 1));
    *((vu16*) 0x09FFFFF0);
    *((vu16*) 0x09FFFFE8);
}

// Opera

static void opera_unlock(uint32_t type) {
    *((vu16*) 0x08240000) = (type & SLOT2_PERIPHERAL_LOCK) ? 0 : 1;
}

static bool opera_detect(void) {
    slot2_extram_start = (uint16_t*) 0x9000000;
    return extram_detect();
}

extern bool paddleIsInserted(void);
extern bool pianoIsInserted(void);
extern bool guitarGripIsInserted(void);

// EZ-Flash

static bool ezf_detect(void) {
    slot2_extram_start = (uint16_t*) 0x8400000;
    if (__extram_detect(1)) return true;
    slot2_extram_start = (uint16_t*) 0x8800000;
    if (__extram_detect(4)) return true;
    return false;
}

static void ezf_unlock(uint32_t type) {
    if (type & SLOT2_PERIPHERAL_LOCK) {
        slot2EzCommand(EZ_CMD_SET_NOR_WRITE, 0xD200); // Disable writing
    } else {
        slot2EzCommand(EZ_CMD_SET_ROM_PAGE, 0x8000); // Enable OS mode
        slot2EzCommand(EZ_CMD_SET_NOR_WRITE, 0x1500); // Enable writing
    }
}

static void ez3in1_unlock(uint32_t type) {
    if (type & SLOT2_PERIPHERAL_LOCK) {
        slot2EzCommand(EZ_CMD_SET_NOR_WRITE, 0xD200); // Disable writing
    } else {
        slot2EzCommand(EZ_CMD_SET_ROM_PAGE, 0x0160); // Map PSRAM
        slot2EzCommand(EZ_CMD_SET_NOR_WRITE, 0x1500); // Enable writing
    }
}

// ED GBA

static void edgba_unlock(uint32_t type) {
    *((vu16*)0x9FC00B4) = 0x00A5;
    *((vu16*)0x9FC0000) = (type & SLOT2_PERIPHERAL_LOCK) ? 0x0 : 0x6;
}

// GPIO

#define GPIO_DIRECTION (*(vuint16 *)0x080000C6)
#define GPIO_CONTROL   (*(vuint16 *)0x080000C8)

static void gpio_unlock(uint32_t type) {
    if (type & SLOT2_PERIPHERAL_LOCK) {
        GPIO_CONTROL = 0;
        GPIO_DIRECTION = 0;
    } else if (type & SLOT2_PERIPHERAL_GYRO_GPIO) {
        GPIO_CONTROL = 1;
        GPIO_DIRECTION = 0xD; // 0b1101
    } else if (type & SLOT2_PERIPHERAL_RUMBLE_GPIO) {
        GPIO_CONTROL = 0;
        GPIO_DIRECTION = 0x8; // 0b1000
    } else if (type & SLOT2_PERIPHERAL_SOLAR_GPIO) {
        GPIO_CONTROL = 1;
        GPIO_DIRECTION = 0x7; // 0b0111
    }
}

static slot2_definition_t definitions[] = {
    // SuperCard
    {
        0x53534150, // "PASS"
        SLOT2_PERIPHERAL_EXTRAM | SLOT2_PERIPHERAL_RUMBLE_GPIO,
        SLOT2_EXMEMCNT_4_2,
        0,
        supercard_detect,
        supercard_unlock
    },
    // M3
    {
        0,
        SLOT2_PERIPHERAL_EXTRAM,
        SLOT2_EXMEMCNT_3_1,
        0,
        extram_detect,
        m3_unlock
    },
    // G6
    {
        0,
        SLOT2_PERIPHERAL_EXTRAM,
        SLOT2_EXMEMCNT_3_1,
        0,
        extram_detect,
        g6_unlock
    },
    // EZ3, EZ4, EZO, EZODE
    {
        0,
        SLOT2_PERIPHERAL_EXTRAM | SLOT2_PERIPHERAL_RUMBLE_PAK | SLOT2_PERIPHERAL_RUMBLE_EZ,
        SLOT2_EXMEMCNT_2_1,
        0,
        ezf_detect,
        ezf_unlock
    },
    // EZF 3in1
    {
        0,
        SLOT2_PERIPHERAL_EXTRAM | SLOT2_PERIPHERAL_RUMBLE_PAK | SLOT2_PERIPHERAL_RUMBLE_EZ,
        SLOT2_EXMEMCNT_2_1,
        0,
        ezf_detect,
        ez3in1_unlock
    },
    // ED GBA
    {
        0,
        SLOT2_PERIPHERAL_EXTRAM,
        SLOT2_EXMEMCNT_2_1,
        0,
        extram_detect,
        edgba_unlock
    },
    // Opera
    {
        0,
        SLOT2_PERIPHERAL_EXTRAM,
        SLOT2_EXMEMCNT_4_2,
        0,
        opera_detect,
        opera_unlock
    },
    // Paddle
    {
        0,
        SLOT2_PERIPHERAL_PADDLE,
        EXMEMCNT_ROM_TIME1_10_CYCLES | EXMEMCNT_ROM_TIME2_6_CYCLES | EXMEMCNT_SRAM_TIME_18_CYCLES | EXMEMCNT_PHI_CLOCK_4MHZ,
        0,
        paddleIsInserted,
        none_unlock
    },
    // Piano
    {
        0,
        SLOT2_PERIPHERAL_PIANO,
        SLOT2_EXMEMCNT_4_2,
        0,
        pianoIsInserted,
        none_unlock
    },
    // Rumble Pak
    {
        0,
        SLOT2_PERIPHERAL_RUMBLE_PAK,
        SLOT2_EXMEMCNT_4_2,
        0,
        pak_rumble_detect,
        none_unlock
    },
    // Guitar Grip
    {
        0,
        SLOT2_PERIPHERAL_GUITAR_GRIP,
        EXMEMCNT_ROM_TIME1_18_CYCLES | EXMEMCNT_ROM_TIME2_6_CYCLES | EXMEMCNT_SRAM_TIME_10_CYCLES | EXMEMCNT_PHI_CLOCK_OFF,
        0,
        guitarGripIsInserted,
        none_unlock
    },
    // WarioWare (GPIO Rumble + Gyro)
    {
        0x00575a52, // "RZW_"
        SLOT2_PERIPHERAL_RUMBLE_GPIO | SLOT2_PERIPHERAL_GYRO_GPIO,
        SLOT2_EXMEMCNT_4_2,
        0,
        none_detect,
        gpio_unlock
    },
    // Drill Dozer (GPIO Rumble)
    {
        0x00393456, // "V49_"
        SLOT2_PERIPHERAL_RUMBLE_GPIO,
        SLOT2_EXMEMCNT_4_2,
        0,
        none_detect,
        gpio_unlock
    },
    // Boktai 1 (GPIO Solar)
    {
        0x00313355, // "U31_"
        SLOT2_PERIPHERAL_SOLAR_GPIO,
        SLOT2_EXMEMCNT_4_2,
        0,
        none_detect,
        gpio_unlock
    },
    // Boktai 2 (GPIO Solar)
    {
        0x00323355, // "U32_"
        SLOT2_PERIPHERAL_SOLAR_GPIO,
        SLOT2_EXMEMCNT_4_2,
        0,
        none_detect,
        gpio_unlock
    },
    // Boktai 3 (GPIO Solar)
    {
        0x00333355, // "U33_"
        SLOT2_PERIPHERAL_SOLAR_GPIO,
        SLOT2_EXMEMCNT_4_2,
        0,
        none_detect,
        gpio_unlock
    },
    // Koro Koro Puzzle (GPIO Tilt)
    {
        0x4a50484b, // "KHPJ"
        SLOT2_PERIPHERAL_TILT,
        SLOT2_EXMEMCNT_4_2,
        0,
        none_detect,
        none_unlock
    },
    // Yoshi (GPIO Tilt)
    {
        0x0047594b, // "KYG_"
        SLOT2_PERIPHERAL_TILT,
        SLOT2_EXMEMCNT_4_2,
        0,
        none_detect,
        none_unlock
    },
};
#define DEFINITIONS_COUNT (sizeof(definitions) / sizeof(slot2_definition_t))

static const char *definition_names[] = {
    "SuperCard",
    "M3",
    "G6",
    "EZ-Flash III/IV/Omega",
    "EZ-Flash 3in1",
    "EverDrive GBA",
    "DS Memory Expansion Pak",
    "Paddle Controller",
    "Easy Piano",
    "DS Rumble Pak",
    "Guitar Grip",
    "GBA Rumble/Gyro",
    "GBA Rumble",
    "GBA Solar",
    "GBA Solar",
    "GBA Solar",
    "GBA Tilt",
    "GBA Tilt"
};

// Public methods

const char *peripheralSlot2GetName(void) {
    if (isDSiMode())
        return slot2_extram_size ? "TWL Debug RAM" : "None";
    if (slot2_device_id == 0xFFFF)
        return "None";
    return definition_names[slot2_device_id];
}

bool peripheralSlot2IsDetected(void) {
    return slot2_device_id != 0xFFFF || slot2_extram_size;
}

uint32_t peripheralSlot2GetSupportMask(void) {
    if (isDSiMode())
        return slot2_extram_size ? SLOT2_PERIPHERAL_EXTRAM : 0;
    return slot2_device_id == 0xFFFF ? 0 : definitions[slot2_device_id].peripheral_mask;
}

void peripheralSlot2Open(uint32_t peripheral_mask) {
    if (slot2_device_id != 0xFFFF) {
        sysSetCartOwner(BUS_OWNER_ARM9);
        definitions[slot2_device_id].unlock(peripheral_mask & definitions[slot2_device_id].peripheral_mask);
    }
}

void peripheralSlot2Close(void) {
    if (slot2_device_id != 0xFFFF)
        definitions[slot2_device_id].unlock(SLOT2_PERIPHERAL_LOCK);
}

void peripheralSlot2Exit(void) {
    slot2_device_id = 0xFFFF;
    slot2_extram_size = 0;
    slot2_extram_banks = 0;
}

extern bool slot2DetectTWLDebugRam(void);

bool peripheralSlot2Init(uint32_t peripheral_mask) {
    peripheralSlot2Exit();

    if (isDSiMode()) {
        if (peripheral_mask & SLOT2_PERIPHERAL_EXTRAM) {
            if (slot2DetectTWLDebugRam()) {
                slot2_extram_size = 0x1000000;
                slot2_extram_start = (uint16_t*) 0xD000000;
                return true;
            }
        }

        return false;
    }

    for (uint32_t i = 0; i < DEFINITIONS_COUNT; i++) {
        slot2_definition_t *def = &definitions[i];
        // Check if any peripherals match
        if (!(def->peripheral_mask & peripheral_mask))
            continue;
        // Update EXMEMCNT
        REG_EXMEMCNT = (REG_EXMEMCNT & ~0xFF) | def->exmemcnt;
        // Check if game code matches
        if (def->gamecode) {
            if (GBA_HEADER.is96h != 0x96)
                continue;
            if (def->gamecode & 0xFF000000) {
                // 4-byte check
                if (*((uint32_t*) GBA_HEADER.gamecode) != def->gamecode)
                    continue;
            } else {
                // 3-byte check (ignore last character)
                if ((*((uint32_t*) GBA_HEADER.gamecode) & 0x00FFFFFF) != (def->gamecode & 0x00FFFFFF))
                    continue;
            }
        }
        // Update flags
        slot2_extram_start = (uint16_t*) 0x8000000;
        // Run detection method
        def->unlock(def->peripheral_mask);
        if (def->detect()) {
            slot2_device_id = i;
            // Open for the user-requested peripheral mask.
            if ((def->peripheral_mask & peripheral_mask) != def->peripheral_mask) {
                def->unlock(SLOT2_PERIPHERAL_LOCK);
                peripheralSlot2Open(peripheral_mask);
            }
            return true;
        }
        def->unlock(SLOT2_PERIPHERAL_LOCK);
    }

    return false;
}

// Public definitions (extRAM)

uint16_t *peripheralSlot2RamStart(void) {
    if (!slot2_extram_size)
        return NULL;
    return slot2_extram_start;
}

uint32_t peripheralSlot2RamSize(void) {
    return slot2_extram_size;
}

uint32_t peripheralSlot2RamBanks(void) {
    return slot2_extram_banks;
}

void peripheralSlot2RamSetBank(uint32_t bank) {
    if (slot2_extram_banks > 1) {
        // Currently, only the EZO/EZODE supports more than one bank.
        // This will need to be refactored if that changes.
        slot2EzCommand(EZ_CMD_SET_PSRAM_PAGE, bank << 12);
    }
}
