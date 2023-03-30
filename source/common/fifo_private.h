// SPDX-License-Identifier: Zlib
//
// Copyright (c) 2008-2015 Dave Murphy (WinterMute)

#ifndef FIFO_PRIVATE_H__
#define FIFO_PRIVATE_H__

#include <nds/ndstypes.h>

// Some aspects of this configuration can be changed...

// FIFO_CHANNEL_BITS - number of bits used to specify the channel in a packet - default=4
#define FIFO_CHANNEL_BITS				4

// FIFO_MAX_DATA_WORDS - maximum number of bytes that can be sent in a fifo message
#define FIFO_MAX_DATA_BYTES				128

// FIFO_RIGOROUS_ERROR_CHECKING - Verify all internal buffer transactions, mostly for debugging this library. Unless there's memory corruption this shouldn't be enabled normally.
// If there is an error, the lib will call int fifoError(char *, ...) - which isn't defined by the fifo lib. So it's best to handle it if you want to debug. :)
// All of the errors trapped represent serious problems, so it's not a bad idea to halt in fifoError()
// #define FIFO_RIGOROUS_ERROR_CHECKING	1

// FIFO_BUFFER_ENTRIES - number of words that can be stored temporarily while waiting to deque them
#ifdef ARM9
#define FIFO_BUFFER_ENTRIES				256
#else // ARM7
#define FIFO_BUFFER_ENTRIES				256
#endif

// Note about memory commitments:
// The memory overhead of this library (per CPU) is:
// 16 + (Num Channels)*32 + FIFO_BUFFER_ENTRIES*8
// for 16 channels and 256 entries, this is 16+512+2048 = 2576 bytes of ram.
// Some padding may be added by the compiler, though.

// Common interface: Both the arm7 and arm9 have the same set of functions.
// for sending/receiving, true=success - for checking things, true=exists

// And some aspects of the configuration can't be changed
//  please don't edit below this line.

#define FIFO_ADDRESSDATA_SHIFT			0
#define FIFO_MINADDRESSDATABITS			24
#define FIFO_ADDRESSDATA_MASK			0x00FFFFFF
#define FIFO_ADDRESSBASE				0x02000000
#define FIFO_ADDRESSCOMPATIBLE			0xFF000000

#define FIFO_NUM_CHANNELS				(1<<FIFO_CHANNEL_BITS)
#define FIFO_CHANNEL_SHIFT				(32-FIFO_CHANNEL_BITS)
#define FIFO_CHANNEL_MASK				((1<<FIFO_CHANNEL_BITS)-1)


// addressbit indicates presence of an address
#define FIFO_ADDRESSBIT_SHIFT			(FIFO_CHANNEL_SHIFT-1)
#define FIFO_ADDRESSBIT					(1<<FIFO_ADDRESSBIT_SHIFT)
// immediatebit indicates presence of an immediate, if there is no address.
#define FIFO_IMMEDIATEBIT_SHIFT			(FIFO_CHANNEL_SHIFT-2)
#define FIFO_IMMEDIATEBIT				(1<<FIFO_IMMEDIATEBIT_SHIFT)
// extrabit indicates presence of an extra word for an immediate.
#define FIFO_EXTRABIT_SHIFT				(FIFO_CHANNEL_SHIFT-3)
#define FIFO_EXTRABIT					(1<<FIFO_EXTRABIT_SHIFT)


#define FIFO_VALUE32_MASK				(FIFO_EXTRABIT-1)


#define FIFO_BUFFER_TERMINATE	0xFFFF
#define FIFO_BUFFER_NEXTMASK	0xFFFF

// some guards to prevent misuse
#if ((FIFO_MINADDRESSDATABITS + FIFO_CHANNEL_BITS + 1) > 32)
#error "Too many channel bits - control word isn't big enough for address packet"
#endif


// some helpers

#define FIFO_PACK_ADDRESS(channel, address) \
	( ((channel)<<FIFO_CHANNEL_SHIFT) | FIFO_ADDRESSBIT | \
	  (((u32)(address)>>FIFO_ADDRESSDATA_SHIFT)&FIFO_ADDRESSDATA_MASK) )

#define FIFO_VALUE32_NEEDEXTRA(value32) \
	( ((value32)&(~FIFO_VALUE32_MASK))!=0 )

#define FIFO_PACK_VALUE32(channel, value32) \
	( ((channel)<<FIFO_CHANNEL_SHIFT) | FIFO_IMMEDIATEBIT | \
	(((value32))&FIFO_VALUE32_MASK) )

#define FIFO_PACK_VALUE32_EXTRA(channel) \
	( ((channel)<<FIFO_CHANNEL_SHIFT) | FIFO_IMMEDIATEBIT | FIFO_EXTRABIT )

#define FIFO_IS_ADDRESS_COMPATIBLE(address) \
	( ((u32)(address)&FIFO_ADDRESSCOMPATIBLE) == FIFO_ADDRESSBASE )

#define FIFO_PACK_DATAMSG_HEADER(channel, numbytes) \
	( ((channel)<<FIFO_CHANNEL_SHIFT) | ((numbytes)&FIFO_VALUE32_MASK) )


#define FIFO_IS_ADDRESS(dataword) (((dataword)&FIFO_ADDRESSBIT)!=0)

#define FIFO_IS_VALUE32(dataword) \
	( (((dataword)&FIFO_ADDRESSBIT)==0) && (((dataword)&FIFO_IMMEDIATEBIT)!=0) )

#define FIFO_IS_DATA(dataword) \
	(((dataword)&(FIFO_ADDRESSBIT|FIFO_IMMEDIATEBIT))==0)

#define FIFO_UNPACK_CHANNEL(dataword) \
	( ((dataword)>>FIFO_CHANNEL_SHIFT)&FIFO_CHANNEL_MASK )

#define FIFO_UNPACK_ADDRESS(dataword) \
	( (void *)((((dataword)&FIFO_ADDRESSDATA_MASK)<<FIFO_ADDRESSDATA_SHIFT) | FIFO_ADDRESSBASE) )

#define FIFO_UNPACK_VALUE32_NEEDEXTRA(dataword) \
	( ((dataword)&FIFO_EXTRABIT)!=0 )

#define FIFO_UNPACK_VALUE32_NOEXTRA(dataword) \
	((dataword)&FIFO_VALUE32_MASK)

#define FIFO_UNPACK_DATALENGTH(dataword) \
	((dataword)&FIFO_VALUE32_MASK)

bool fifoInternalSend(u32 firstword, u32 extrawordcount, u32 * wordlist);

// ----------------------------------------------------------------------

#define FIFO_ARM9_REQUESTS_ARM7_RESET   0x4000C
#define FIFO_ARM7_REQUESTS_ARM9_RESET   0x4000B

// ----------------------------------------------------------------------

#endif // FIFO_PRIVATE_H__
