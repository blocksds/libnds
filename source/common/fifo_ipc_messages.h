// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (c) 2008-2015 Dave Murphy (WinterMute)
// Copyright (c) 2023 Antonio Niño Díaz

#ifndef FIFO_IPC_MESSAGES_H__
#define FIFO_IPC_MESSAGES_H__

#include <stdbool.h>

#include <nds/ndstypes.h>

// Defines related to the header block of a FIFO message
// -----------------------------------------------------

// Number of bits used to specify the channel of a packet
#define FIFO_CHANNEL_BITS       4

#define FIFO_NUM_CHANNELS       (1 << FIFO_CHANNEL_BITS)
#define FIFO_CHANNEL_SHIFT      (32 - FIFO_CHANNEL_BITS)
#define FIFO_CHANNEL_MASK       ((1 << FIFO_CHANNEL_BITS) - 1)

// If this bit is set, the message is an address (0x02000000 - 0x02FFFFFF)
#define FIFO_ADDRESSBIT_SHIFT   (FIFO_CHANNEL_SHIFT - 1)
#define FIFO_ADDRESSBIT         (1 << FIFO_ADDRESSBIT_SHIFT)

// If this bit is set, the message is an immediate value.
#define FIFO_IMMEDIATEBIT_SHIFT (FIFO_ADDRESSBIT_SHIFT - 1)
#define FIFO_IMMEDIATEBIT       (1 << FIFO_IMMEDIATEBIT_SHIFT)

// If this bit is set, it means that the provided immediate value doesn't fit in
// a 32-bit header block. In that case, the value is sent in the block right
// after the header.
#define FIFO_EXTRABIT_SHIFT     (FIFO_IMMEDIATEBIT_SHIFT - 1)
#define FIFO_EXTRABIT           (1 << FIFO_EXTRABIT_SHIFT)

// Note: Some special commands can be accessed by setting the address bit and
// the immediate bit at the same time. This isn't normally allowed. Also, if
// both bits are 0, this is a data message of an arbitrary length.

// General message format:
//
// |31 ... 28 |  27  | 26    | 25    | 24 ... 0        || 31 ... 0
// +----------+------+-------+-------+-----------------++-----------------
// | Channel  | Addr | Immed | Extra | Data            || Additional data
//
// Messages of immediate values:
//
// |31 ... 28 |  27  | 26    | 25    | 24 ... 0        || 31 ... 0
// +----------+------+-------+-------+-----------------++-----------------
// | Channel  |  0   |  1    |   0   | Small immediate ||
// | Channel  |  0   |  1    |   1   | X               || 32-bit immediate
//
// Messages of addresses:
//
// |31 ... 28 |  27  | 26    | 25    | 24 ... 0        |
// +----------+------+-------+-------+-----------------+
// | Channel  |  1   |  0    |   X   | Address         |
//
// Messages of data of arbitrary size:
//
// |31 ... 28 |  27  | 26    | 25    | 24 ... 0        || 31 ... 0
// +----------+------+-------+-------+-----------------++-----------------------
// | Channel  |  0   |  0    |   X   | Length (bytes)  || Word 0 (first of many)
//
// Messages of special commands (the channel is ignored):
//
// |31 ... 28 |  27  | 26    | 25    | 24 ... 0        |
// +----------+------+-------+-------+-----------------+
// |   X      |  1   |  1    |   X   | Command         |

static inline uint32_t fifo_ipc_unpack_channel(uint32_t dataword)
{
    return (dataword >> FIFO_CHANNEL_SHIFT) & FIFO_CHANNEL_MASK;
}

// Defines related to 32-bit immediate value messages
// --------------------------------------------------

#define FIFO_VALUE32_MASK   (FIFO_EXTRABIT - 1)

// This returns true if the block is an immediate value (with extra word or not)
static inline bool fifo_ipc_is_value32(uint32_t dataword)
{
    return ((dataword & FIFO_ADDRESSBIT) == 0) &&
           ((dataword & FIFO_IMMEDIATEBIT) != 0);
}

// This returns true if the 32-bit value doesn't fit in one FIFO block. In that
// case, it needs an extra FIFO block.
static inline bool fifo_ipc_value32_needextra(uint32_t value32)
{
    return (value32 & ~FIFO_VALUE32_MASK) != 0;
}

// Returns true if the specified fifo block says it needs an extra word.
static inline bool fifo_ipc_unpack_value32_needextra(uint32_t dataword)
{
    return (dataword & FIFO_EXTRABIT) != 0;
}

// This creates a FIFO message that sends a 32-bit value that fits in one block.
static inline uint32_t fifo_ipc_pack_value32(uint32_t channel, uint32_t value32)
{
    return (channel << FIFO_CHANNEL_SHIFT) | FIFO_IMMEDIATEBIT |
            (value32 & FIFO_VALUE32_MASK);
}

// Extract the small immediate value in messages that don't need an extra word.
static inline uint32_t fifo_ipc_unpack_value32_noextra(uint32_t dataword)
{
    return dataword & FIFO_VALUE32_MASK;
}

// This creates the header of a FIFO message that sends a 32-bit value that
// doesn't fits in one block.
static inline uint32_t fifo_ipc_pack_value32_extra(uint32_t channel)
{
    return (channel << FIFO_CHANNEL_SHIFT) | FIFO_IMMEDIATEBIT | FIFO_EXTRABIT;
}

// Defines related to address messages
// -----------------------------------

#define FIFO_ADDRESSDATA_SHIFT          0
#define FIFO_MINADDRESSDATABITS         24
#define FIFO_ADDRESSDATA_MASK           0x00FFFFFF
#define FIFO_ADDRESSBASE                0x02000000
#define FIFO_ADDRESSCOMPATIBLE          0xFF000000

// This creates a FIFO message that sends an address in one FIFO block.
static inline uint32_t fifo_ipc_pack_address(uint32_t channel, void *address)
{
    return (channel << FIFO_CHANNEL_SHIFT) | FIFO_ADDRESSBIT |
           (((uint32_t)address >> FIFO_ADDRESSDATA_SHIFT) & FIFO_ADDRESSDATA_MASK);
}

// This returns true if the address can be sent as a FIFO address message. It
// needs to be placed in main RAM for it to be compatible.
static inline bool fifo_ipc_is_address_compatible(void *address)
{
    return ((uint32_t)address & FIFO_ADDRESSCOMPATIBLE) == FIFO_ADDRESSBASE;
}

static inline bool fifo_ipc_is_address(uint32_t dataword)
{
    return (dataword & FIFO_ADDRESSBIT) != 0;
}

static inline void *fifo_ipc_unpack_address(uint32_t dataword)
{
    uint32_t address = ((dataword & FIFO_ADDRESSDATA_MASK) << FIFO_ADDRESSDATA_SHIFT)
                     | FIFO_ADDRESSBASE;
    return (void *)address;
}

// Defines related to data messages
// --------------------------------

// This creates the header of a FIFO message that sends an arbitrary number of
// bytes. The actual bytes must be sent right after the header.
static inline uint32_t fifo_ipc_pack_datamsg_header(uint32_t channel, uint32_t numbytes)
{
    return (channel << FIFO_CHANNEL_SHIFT) | (numbytes & FIFO_VALUE32_MASK);
}

static inline bool fifo_ipc_is_data(uint32_t dataword)
{
    return (dataword & (FIFO_ADDRESSBIT | FIFO_IMMEDIATEBIT)) == 0;
}

static inline uint32_t fifo_ipc_unpack_datalength(uint32_t dataword)
{
    return dataword & FIFO_VALUE32_MASK;
}

// Defines related to special commands
// -----------------------------------

#define FIFO_SPECIAL_COMMAND_MASK       0x00FFFFFF

// This returns true if the block is a special command
static inline bool fifo_ipc_is_special_command(uint32_t dataword)
{
    return ((dataword & FIFO_ADDRESSBIT) != 0) &&
           ((dataword & FIFO_IMMEDIATEBIT) != 0);
}

// This creates the header of a FIFO message that sends a special command.
static inline uint32_t fifo_ipc_pack_special_command_header(uint32_t cmd)
{
    // The channel number is ignored
    return FIFO_ADDRESSBIT | FIFO_IMMEDIATEBIT |
           (FIFO_SPECIAL_COMMAND_MASK & cmd);
}

#define FIFO_ARM9_REQUESTS_ARM7_RESET   0x4000C
#define FIFO_ARM7_REQUESTS_ARM9_RESET   0x4000B

#endif // FIFO_IPC_MESSAGES_H__
