// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2005 Dave Murphy (WinterMute)

#include <string.h>

#include <nds/arm7/serial.h>
#include <nds/system.h>

bool readUserSettings(void)
{
    PERSONAL_DATA slots[2];

    struct PACKED {
        short count;
        short crc;
    } slot1Footer, slot2Footer;

    uint16_t userSettingsBase;
    readFirmware(0x20, &userSettingsBase,2);

    uint32_t slot1Address = (uint32_t)userSettingsBase * 8;
    uint32_t slot2Address = (uint32_t)userSettingsBase * 8 + 0x100;

    readFirmware(slot1Address, &slots[0], sizeof(PERSONAL_DATA));
    readFirmware(slot2Address, &slots[1], sizeof(PERSONAL_DATA));
    readFirmware(slot1Address + 0x70, &slot1Footer, 4);
    readFirmware(slot2Address + 0x70, &slot2Footer, 4);

    // default to slot 1 user Settings
    int currentSettingsSlot = 0;

    short calc1CRC = swiCRC16(0xffff, &slots[0], sizeof(PERSONAL_DATA));
    short calc2CRC = swiCRC16(0xffff, &slots[1], sizeof(PERSONAL_DATA));

    // bail out if neither slot is valid
    if (calc1CRC != slot1Footer.crc && calc2CRC != slot2Footer.crc)
        return false;

    // if both slots are valid pick the most recent
    if (calc1CRC == slot1Footer.crc && calc2CRC == slot2Footer.crc)
    {
        currentSettingsSlot = (slot2Footer.count == ((slot1Footer.count + 1) & 0x7f) ? 1 : 0);
    }
    else
    {
        if (calc2CRC == slot2Footer.crc)
            currentSettingsSlot = 1;
    }

    *PersonalData = slots[currentSettingsSlot];
    return true;
}
