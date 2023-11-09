// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2005 Michael Noland (joat)
// Copyright (C) 2005 Jason Rogers (dovoto)
// Copyright (C) 2005 Dave Murphy (WinterMute)

#ifndef LIBNDS_NDS_CARD_H__
#define LIBNDS_NDS_CARD_H__

/// @file nds/card.h
///
/// @brief Slot-1 card commands.

#include <stddef.h>

#include <nds/ndstypes.h>

// Card bus
#define REG_CARD_DATA_RD (*(vu32 *)0x04100010)

#define REG_AUXSPICNT  (*(vu16 *)0x040001A0)
#define REG_AUXSPICNTH (*(vu8 *)0x040001A1)
#define REG_AUXSPIDATA (*(vu8 *)0x040001A2)
#define REG_ROMCTRL    (*(vu32 *)0x040001A4)

#define REG_CARD_COMMAND ((vu8 *)0x040001A8)

#define REG_CARD_1B0 (*(vu32 *)0x040001B0)
#define REG_CARD_1B4 (*(vu32 *)0x040001B4)
#define REG_CARD_1B8 (*(vu16 *)0x040001B8)
#define REG_CARD_1BA (*(vu16 *)0x040001BA)

#define CARD_CR1_ENABLE 0x80 // In byte 1, i.e. 0x8000
#define CARD_CR1_IRQ    0x40 // In byte 1, i.e. 0x4000

// SPI EEPROM COMMANDS
#define SPI_EEPROM_WRSR 0x01
#define SPI_EEPROM_PP   0x02 // Page Program
#define SPI_EEPROM_READ 0x03
#define SPI_EEPROM_WRDI 0x04 // Write disable
#define SPI_EEPROM_RDSR 0x05 // Read status register
#define SPI_EEPROM_WREN 0x06 // Write enable
#define SPI_EEPROM_PW   0x0a // Page Write
#define SPI_EEPROM_FAST 0x0b // Fast Read
#define SPI_EEPROM_RDID 0x9f
#define SPI_EEPROM_RDP  0xab // Release from deep power down
#define SPI_EEPROM_DPD  0xb9 // Deep power down

#define CARD_ACTIVATE     (1 << 31) // When writing, get the ball rolling
#define CARD_WR           (1 << 30) // Card write enable
#define CARD_nRESET       (1 << 29) // Value on the /reset pin (1 = high out, not a reset state, 0 = low out = in reset)
#define CARD_SEC_LARGE    (1 << 28) // Use "other" secure area mode, which tranfers blocks of 0x1000 bytes at a time
#define CARD_CLK_SLOW     (1 << 27) // Transfer clock rate (0 = 6.7MHz, 1 = 4.2MHz)
#define CARD_BLK_SIZE(n)  (((n) & 0x7) << 24) // Transfer block size, (0 = None, 1..6 = (0x100 << n) bytes, 7 = 4 bytes)
#define CARD_SEC_CMD      (1 << 22) // The command transfer will be hardware encrypted (KEY2)
#define CARD_DELAY2(n)    (((n) & 0x3F) << 16)  // Transfer delay length part 2
#define CARD_SEC_SEED     (1 << 15) // Apply encryption (KEY2) seed to hardware registers
#define CARD_SEC_EN       (1 << 14) // Security enable
#define CARD_SEC_DAT      (1 << 13) // The data transfer will be hardware encrypted (KEY2)
#define CARD_DELAY1(n)    ((n) & 0x1FFF) // Transfer delay length part 1

// 3 bits in b10..b8 indicate something
// Read bits
#define CARD_BUSY         (1 << 31) // When reading, still expecting incomming data?
#define CARD_DATA_READY   (1 << 23) // When reading, CARD_DATA_RD or CARD_DATA has another word of data and is good to go

// Card commands
#define CARD_CMD_DUMMY          0x9F
#define CARD_CMD_HEADER_READ    0x00
#define CARD_CMD_HEADER_CHIPID  0x90
#define CARD_CMD_ACTIVATE_BF    0x3C  // Go into blowfish (KEY1) encryption mode
#define CARD_CMD_ACTIVATE_SEC   0x40  // Go into hardware (KEY2) encryption mode
#define CARD_CMD_SECURE_CHIPID  0x10
#define CARD_CMD_SECURE_READ    0x20
#define CARD_CMD_DISABLE_SEC    0x60  // Leave hardware (KEY2) encryption mode
#define CARD_CMD_DATA_MODE      0xA0
#define CARD_CMD_DATA_READ      0xB7
#define CARD_CMD_DATA_CHIPID    0xB8

//REG_AUXSPICNT
#define CARD_ENABLE     BIT(15)
#define CARD_IRQ        BIT(14)
#define CARD_SPI_ENABLE BIT(13)
#define CARD_SPI_BUSY   BIT(7)
#define CARD_SPI_HOLD   BIT(6)
#define CARD_SPI_BAUD_4MHz   0
#define CARD_SPI_BAUD_2MHz   1
#define CARD_SPI_BAUD_1MHz   2
#define CARD_SPI_BAUD_512KHz 3

#define CARD_SPICNTH_ENABLE  (1 << 7) // In byte 1, i.e. 0x8000
#define CARD_SPICNTH_IRQ     (1 << 6) // In byte 1, i.e. 0x4000

#ifdef __cplusplus
extern "C" {
#endif

void enableSlot1(void);
void disableSlot1(void);

/**
 * @brief Write a command to the card interface.
 * 
 * @param command 8-byte command buffer, little endian.
 */
void cardWriteCommand(const u8 *command);

/**
 * @brief Write a ROM command, reading the response via polling (synchronously).
 * 
 * @param flags The ROM control flags to use for the transfer.
 * @param destination The response's destination buffer.
 * @param length The length of the response, in bytes.
 * @param command 8-byte command buffer, little endian.
 */
void cardPolledTransfer(u32 flags, u32 *destination, u32 length, const u8 *command);

/**
 * @brief Perform a ROM command, reading the response via DMA (asynchronously).
 * Note that this function does not wait for the DMA to complete!
 * 
 * @param command 8-byte command buffer, little endian.
 * @param destination The response's destination buffer.
 * @param channel The DMA channel to use for the transfer.
 * @param flags The ROM control flags to use for the transfer.
 */
void cardStartTransfer(const u8 *command, u32 *destination, int channel, u32 flags);

/**
 * @brief Perform a ROM command, reading one word of response.
 * 
 * @param command 8-byte command buffer, little endian.
 * @param flags The ROM control flags to use for the transfer.
 * @return uint32_t The response.
 */
uint32_t cardWriteAndRead(const u8 *command, u32 flags);

/**
 * @brief Write a ROM command of the following form, reading the response via
 * polling (synchronously):
 *
 * ccpppppppp000000
 *
 * where cc is the command and pp is the parameter.
 * 
 * @param command The command.
 * @param parameter The parameter.
 * @param flags The ROM control flags to use for the transfer.
 * @param destination The response's destination buffer.
 * @param length The length of the response, in bytes.
 */
void cardParamCommand(u8 command, u32 parameter, u32 flags, u32 *destination, u32 length);

// These commands require the cart to not be initialized yet, which may mean the
// user needs to eject and reinsert the cart or they will return random data.
void cardReadHeader(u8 *header);

u32 cardReadID(u32 flags);

void cardReset(void);

/**
 * @brief Read bytes from the card ROM.
 * 
 * @param dest The destination buffer.
 * @param offset The offset to read from, in bytes.
 * @param len The number of bytes to read.
 * @param flags The read flags.
 */
void cardRead(void *dest, size_t offset, size_t len, uint32_t flags);

static inline void eepromWaitBusy(void)
{
    while (REG_AUXSPICNT & CARD_SPI_BUSY);
}

/**
 * @brief Read from the card EEPROM.
 * 
 * @param address The address to read from.
 * @param data The data to write.
 * @param length The length of data, in bytes.
 * @param addrtype The card EEPROM's type. @see cardEepromGetType
 */
void cardReadEeprom(u32 address, u8 *data, u32 length, u32 addrtype);

/**
 * @brief Write to the card EEPROM.
 *
 * Note that TYPE 3 (FLASH) EEPROM must be erased before writing.
 * 
 * @param address The address to write to.
 * @param data The data to write.
 * @param length The length of data, in bytes.
 * @param addrtype The card EEPROM's type. @see cardEepromGetType
 */
void cardWriteEeprom(u32 address, u8 *data, u32 length, u32 addrtype);

/**
 * @brief Attempt to read the ID of the card EEPROM chip.
 * Doesn't work well; most chips return 0xFFFF.
 * 
 * @return u32 The ID of the chip.
 */
u32 cardEepromReadID(void);

/**
 * @brief Send a command to the card EEPROM.
 * 
 * @param command The command to send.
 * @return u8 The result, if any.
 */
u8 cardEepromCommand(u8 command);

/**
 * @brief Read the card EEPROM's type.
 * 
 * @return int The type:
 *   -1: no card or no EEPROM
 *    0: unknown                   PassMe?
 *    1: TYPE 1   4Kbit(512Byte)   EEPROM
 *    2: TYPE 2  64Kbit(8KByte)or  512kbit(64Kbyte)   EEPROM
 *    3: TYPE 3   2Mbit(256KByte)  FLASH MEMORY (some rare 4Mbit and 8Mbit chips also)
 */
int cardEepromGetType(void);

/**
 * @brief Read the card EEPROM's size.
 * 
 * @return u32 The EEPROM's size, in bytes.
 */
u32 cardEepromGetSize(void);

/**
 * @brief Erase the entirety of a TYPE 3 (FLASH) card EEPROM.
 */
void cardEepromChipErase(void);

/**
 * @brief Erase a single sector of a TYPE 3 (FLASH) card EEPROM.
 *
 * @param address The address to erase at.
 */
void cardEepromSectorErase(u32 address);

#ifdef __cplusplus
}
#endif

#endif // LIBNDS_NDS_CARD_H__
