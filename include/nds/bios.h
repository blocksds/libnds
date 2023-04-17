// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2005 Michael Noland (joat)
// Copyright (C) 2005 Jason Rogers (dovoto)
// Copyright (C) 2005 Dave Murphy (WinterMute)

// BIOS functions

#ifndef LIBNDS_NDS_BIOS_H__
#define LIBNDS_NDS_BIOS_H__

/// @file bios.h
///
/// @brief Nintendo DS BIOS functions
///
/// See gbatek for more information.

#ifdef __cplusplus
extern "C" {
#endif

#include <nds/ndstypes.h>

/// Should return the header of a compressed stream of bytes.
///
/// The result is a word, with the size of decompressed data in bits 8 - 31,
/// and bits 0 - 7 are ignored. This value is also returned by the bios
/// function, unless getResult is non-NULL and returns a negative value. This
/// useally returns the 4 bytes that source points to.
///
/// @param source A pointer to the compressed data.
/// @param dest A pointer to the space where the decompressed data should be
///             copied to.
/// @param arg A callback value that gets passed to the bios function.
/// @return The header of the compressed data containing the length of the data
///         and the compression type.
typedef int (*getHeaderCallback)(u8 *source, u16 *dest, u32 arg);

/// Should verify the result after data got decompressed.
///
/// getResult is used to provide a result for the bios function, given the
/// source pointer after all data has been read (or if getSize < 0). Its value
/// is only returned if negative, otherwise the typical result is used, so it is
/// likely some sort of error-checking procedure.
///
/// @param source The current source address.
/// @return 0 if it went right, or a negative number if something went wrong.
///         The value will be returned from BIOS function if value is negative.
typedef int (*getResultCallback)(u8 *source);

/// Should returns a raw byte of the stream.
///
/// @param source A pointer to the byte.
/// @return A byte.
typedef u8 (*getByteCallback)(u8 *source);

/// A struct that contains callback function pointers used by the decompression
/// functions.
typedef struct DecompressionStream
{
    getHeaderCallback getSize;   ///< gets called to get the header of the stream.
    getResultCallback getResult; ///< gets called to verify the result afterwards, can be NULL (no callback).
    getByteCallback readByte;    ///< gets called to get a byte of the compressed data.
    // According to gbatek, there are 2 more callback pointers here.
} PACKED TDecompressionStream;

/// A struct and struct pointer with information about unpacking data.
typedef struct UnpackStruct
{
    uint16 sourceSize; ///< in bytes
    uint8 sourceWidth; ///< 1,2,4 or 8 bits.
    uint8 destWidth;   ///< 1,2,4,8,16 or 32 bits.
    uint32 dataOffset; ///< bits 0-30 are added to all non-zero destination writes, unless bit 31 is set, which does it for zeros too.
} PACKED TUnpackStruct, * PUnpackStruct;

/// Resets the DS.
void swiSoftReset(void);

/// Delays the code.
///
/// Delays for for a period "X + Y * duration" where X is the swi overhead and Y
/// is a cycle of Thumb fetches in BIOS memory.
/// <CODE><PRE>
///      loop:
///        sub r0, #1
///        bgt loop
/// </PRE></CODE>
///
/// @param duration Length of delay.
/// @note Duration should be 1 or more, a duration of 0 is a huge delay.
void swiDelay(uint32 duration);

/// Divides 2 numbers.
///
/// @param numerator Signed integer to divide.
/// @param divisor Signed integer to divide by.
/// @return Numerator / divisor
int swiDivide(int numerator, int divisor);

/// Calculate the remainder of an division.
///
/// @param numerator Signed integer to divide
/// @param divisor Signed integer to divide by
/// @return Numerator % divisor
int swiRemainder(int numerator, int divisor);

/// Divides 2 numbers and stores both the result and the remainder.
///
/// @param numerator Signed integer to divide.
/// @param divisor Signed integer to divide by.
/// @param result Pointer to integer set to numerator / divisor.
/// @param remainder Pointer to integer set to numerator % divisor.
void swiDivMod(int numerator, int divisor, int *result, int *remainder);

/// Copy in chunks of halfword size.
#define COPY_MODE_HWORD        (0)
/// Copy in chunks of word size.
#define COPY_MODE_WORD        BIT(26)
/// copy a range of memory to another piece of memory
#define COPY_MODE_COPY        (0)
/// Fill a piece of memory with a value.
#define COPY_MODE_FILL        BIT(24)

/// Copies or fills some memory.
///
/// @param source Pointer to transfer source or pointer to value to fill the
///               memory with.
/// @param dest Pointer to transfer destination.
/// @param flags bits(0-20): size of data to copy/fill in words, or'd with the
///              copy mode size (word or halfword) and type (copy or fill).
void swiCopy(const void *source, void *dest, int flags);

/// Copies or fills some memory.
///
/// Can only copy in word chunks.
///
/// @param source Pointer to transfer source or pointer to value to fill the
///               memory with.
/// @param dest Pointer to transfer destination.
/// @param flags bits(0-20): size of data to copy/fill in words, or'd with the
///              type (copy or fill).
/// @note Transfers more quickly than swiCopy, but has higher interrupt latency.
void swiFastCopy(const void *source, void *dest, int flags);

/// Calculates the square root.
///
/// @param value The value to calculate.
/// @return The square root of the value as an integer.
/// @note Use fixed point math if you want more accuracy.
int swiSqrt(int value);

/// Calculates a CRC-16 checksum.
///
/// @param crc Starting CRC-16 value.
/// @param data Pointer to data (processed nibble by nibble)
/// @param size Size in bytes.
/// @return The CRC-16 after the data has been processed.
uint16 swiCRC16(uint16 crc, const void *data, uint32 size);

/// Returns 0 if running on a Nintendo hardware debugger.
///
/// @return 0 if running on a debugger (8 MB of RAM instead of 4 MB), else some
///         other number.
int swiIsDebugger(void);

/// Unpack data stored in multiple elements in a byte to a larger space.
///
/// i.e. 8 elements per byte (i.e. b/w font), into 1 element per byte.
///
/// @param source Source address.
/// @param destination Destination address (word aligned).
/// @param params Pointer to an UnpackStruct.
void swiUnpackBits(const uint8 *source, uint32 *destination, PUnpackStruct params);

/// Decompresses LZSS compressed data.
///
/// @param source Pointer to a header word, followed by compressed data. bit 0-7
///               of header is ignored. bit 8-31 of header is size of
///               uncompressed data in bytes.
/// @param destination Destination address.
/// @note Writes data a byte at a time.
/// @see decompress.h
void swiDecompressLZSSWram(const void *source, void *destination);

/// Decompresses LZSS compressed data vram safe.
///
/// @param source Pointer to source data (always goes through the function
///               pointers, so could just be an offset).
/// @param destination Pointer to destination.
/// @param toGetSize Callback value that is passed to getHeaderCallback function
///                  pointer.
/// @param stream Pointer to struct with callback function pointers.
/// @return The length of the decompressed data, or a signed errorcode from the
///         Open/Close functions.
/// @note Writes data a halfword at a time.
/// @see decompress.h
int swiDecompressLZSSVram(const void *source, void *destination, uint32 toGetSize,
                          TDecompressionStream *stream);

int swiDecompressLZSSVramNTR(const void *source, void *destination, uint32 toGetSize,
                             TDecompressionStream *stream);

int swiDecompressLZSSVramTWL(const void *source, void *destination, uint32 toGetSize,
                             TDecompressionStream *stream);

/// Decompresses Huffman compressed data.
///
/// @param source Pointer to source data (always goes through the function
///               pointers, so could just be an offset).
/// @param destination Pointer to destination.
/// @param toGetSize Callback value that is passed to getHeaderCallback function
///                  pointer.
/// @param stream Pointer to struct with callback function pointers.
/// @return The length of the decompressed data, or a signed errorcode from the
///         Open/Close functions.
/// @see decompress.h
int swiDecompressHuffman(const void *source, void *destination, uint32 toGetSize,
                         TDecompressionStream *stream);

/// Decompresses RLE compressed data.
///
/// Compressed data format:
/// - bit(7): 0= uncompressed, 1= compressed.
/// - bit(0-6) when uncompressed: run length - 1, followed by run_length bytes
///   of true data.
/// - bit(0-6) when compressed: run length - 3, followed by one byte of true
///   data, to be repeated.
///
/// @param source Pointer to a header word, followed by compressed data. bit 0-7
///               of header is ignored. bit 8-31 of header is size of
///               uncompressed data in bytes.
/// @param destination Destination address.
/// @note Writes data a byte at a time.
/// @see decompress.h
void swiDecompressRLEWram(const void *source, void *destination);

/// Decompresses RLE compressed data vram safe.
///
/// Compressed data format:
/// - bit(7): 0 = uncompressed, 1 = compressed.
/// - bit(0-6) when uncompressed: run length - 1, followed by run_length bytes
///   of true data.
/// - bit(0-6) when compressed: run length - 3, followed by one byte of true
///   data, to be repeated.
///
/// @param source Pointer to source data (always goes through the function
///               pointers, so could just be an offset).
/// @param destination Pointer to destination.
/// @param toGetSize Callback value that is passed to getHeaderCallback function
///                  pointer.
/// @param stream Pointer to struct with callback function pointers.
/// @return The length of the decompressed data, or a signed errorcode from the
///         Open/Close functions.
/// @note Writes data a halfword at a time.
/// @see decompress.h
int swiDecompressRLEVram(const void *source, void *destination, uint32 toGetSize,
                         TDecompressionStream *stream);

#ifdef ARM9

/// Wait for any interrupt.
///
/// @note ARM9 exclusive.
void swiWaitForIRQ(void);

/// Writes a word of the data to 0x04000300:32
///
/// @param data The word to write.
/// @note This is on the ARM9, but works differently then the ARM7 function!
void swiSetHaltCR(uint32 data);

/// Decodes a stream of bytes based on the difference of the bytes.
///
/// @param source Pointer to a header word, followed by encoded data.
///               word(31..8) = size of data (in bytes).
///               word(7..0) = ignored.
/// @param destination Destination address.
/// @note Writes data a byte at a time.
/// @note ARM9 exclusive.
void swiDecodeDelta8(const void *source, void *destination);

/// Decodes a stream of bytes based on the difference of the bytes.
///
/// @param source Pointer to a header word, followed by encoded data.
///               word(31..8) = size of data (in bytes).
///               word(7..0) = ignored.
/// @param destination Destination address.
/// @note Writes data a halfword at a time.
/// @note ARM9 exclusive.
void swiDecodeDelta16(const void *source, void *destination);

#endif

#ifdef ARM7

/// Writes a byte of the data to 0x04000301:8
///
/// @param data The byte to write.
/// @note ARM7 exclusive.
void swiSetHaltCR(uint8 data);

/// Halts the CPU until an interupt occures.
///
/// @note ARM7 exclusive.
void swiHalt(void);

/// Halts the CPU and most of the hardware untill an interupt occures.
///
/// @note ARM7 exclusive.
void swiSleep(void);

/// Switches the DS to GBA mode.
///
/// @note ARM7 exclusive.
void swiSwitchToGBAMode(void);

/// Returns an entry in the sine table.
///
/// @param index The index of the sine table (0 - 63).
/// @return The entry.
/// @note ARM7 exclusive.
uint16 swiGetSineTable(int index);

/// Returns an entry in the pitch table.
///
/// @param index The index of the pitch table (0 - 767).
/// @return The entry.
/// @note ARM7 exclusive.
uint16 swiGetPitchTable(int index);

/// Returns an entry in the volume table.
///
/// @param index The index of the volume table (0 - 723).
/// @return The entry.
/// @note ARM7 exclusive.
uint8 swiGetVolumeTable(int index);

/// Increments or decrements the sound bias once per delay.
///
/// @param enabled Set to 0 to decrement it until it reaches 0x000, set to 1 to
///                increment it until it reaches 0x200.
/// @param delay Is in the same units of time as swiDelay.
/// @note ARM7 exclusive.
void swiChangeSoundBias(int enabled, int delay);

#endif // ARM7

#ifdef __cplusplus
}
#endif

#endif // LIBNDS_NDS_BIOS_H__
