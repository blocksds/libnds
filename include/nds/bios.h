// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2005 Michael Noland (joat)
// Copyright (C) 2005 Jason Rogers (dovoto)
// Copyright (C) 2005 Dave Murphy (WinterMute)
// Copyright (C) 2022-2023 gba-hpp contributors

// BIOS functions

#ifndef LIBNDS_NDS_BIOS_H__
#define LIBNDS_NDS_BIOS_H__

#ifdef __cplusplus
extern "C" {
#endif

/// @file nds/bios.h
///
/// @brief Nintendo DS BIOS functions
///
/// See gbatek for more information.

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
/// getResultCallback is used to provide a result for the bios function, given
/// the source pointer after all data has been read (or if getSize < 0). Its
/// value is only returned if negative, otherwise the typical result is used, so
/// it is likely some sort of error-checking procedure.
///
/// @param source The current source address.
/// @return 0 if it went right, or a negative number if something went wrong.
///         The value will be returned from BIOS function if value is negative.
typedef int (*getResultCallback)(u8 *source);

/// Should return a raw byte of the stream.
///
/// @param source A pointer to the byte.
/// @return A byte.
typedef u8 (*getByteCallback)(u8 *source);

/// Should return a raw halfword of the stream.
///
/// @param source A pointer to the halfword.
/// @return A halfword.
typedef u16 (*getHalfWordCallback)(u16 *source);

/// Should return a raw word of the stream.
///
/// @param source A pointer to the word.
/// @return A word.
typedef u32 (*getWordCallback)(u32 *source);

/// A struct that contains callback function pointers used by the decompression
/// functions.
typedef struct DecompressionStream
{
    /// It gets called to get the header of the stream.
    getHeaderCallback getSize;
    /// It gets called to verify the result afterwards. It can be NULL.
    getResultCallback getResult;
    /// It gets called to get a byte of the compressed data.
    getByteCallback readByte;
    /// It gets called to get a halfword of the compressed data. Unused.
    getHalfWordCallback readHalfWord;
    /// It gets called to get a word of the compressed data. Used for Huffman.
    getWordCallback readWord;
} PACKED TDecompressionStream;

/// A struct and struct pointer with information about unpacking data.
typedef struct UnpackStruct
{
    uint16_t sourceSize; ///< in bytes
    uint8_t sourceWidth; ///< 1,2,4 or 8 bits.
    uint8_t destWidth;   ///< 1,2,4,8,16 or 32 bits.

    /// Bits 0-30 are added to all non-zero destination writes. If bit 31 is
    /// set they are added to zeroes too.
    uint32_t dataOffset;
} PACKED TUnpackStruct, __attribute__((deprecated)) *PUnpackStruct;

/// Resets the DS.
#ifdef __clang__
__attribute__((noreturn))
void swiSoftReset(void);
#else
__attribute__((always_inline, noreturn))
static inline void swiSoftReset(void)
{
    asm volatile inline ("swi 0x0 << ((1f - . == 4) * -16); 1:");
    __builtin_unreachable();
}
#endif

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
#ifdef __clang__
void swiDelay(uint32_t duration);
#else
__attribute__((always_inline))
static inline void swiDelay(uint32_t duration)
{
    register uint32_t r0 asm("r0") = duration;
    asm volatile inline ("swi 0x3 << ((1f - . == 4) * -16); 1:" : "+r"(r0) :: "r1", "r3");
}
#endif

/// Divides 2 numbers.
///
/// @param numerator Signed integer to divide.
/// @param divisor Signed integer to divide by.
/// @return Numerator / divisor
#ifdef __clang__
int swiDivide(int numerator, int divisor);
#else
__attribute__((always_inline))
static inline int swiDivide(int numerator, int divisor)
{
    register int r0 asm("r0") = numerator;
    register int r1 asm("r1") = divisor;
    asm volatile inline ("swi 0x9 << ((1f - . == 4) * -16); 1:" : "+r"(r0), "+r"(r1) :: "r3");
    return r0;
}
#endif

/// Calculate the remainder of an division.
///
/// @param numerator Signed integer to divide
/// @param divisor Signed integer to divide by
/// @return Numerator % divisor
#ifdef __clang__
int swiRemainder(int numerator, int divisor);
#else
__attribute__((always_inline))
static inline int swiRemainder(int numerator, int divisor)
{
    register int r0 asm("r0") = numerator;
    register int r1 asm("r1") = divisor;
    asm volatile inline ("swi 0x9 << ((1f - . == 4) * -16); 1:" : "+r"(r0), "+r"(r1) :: "r3");
    return r1;
}
#endif

/// Divides 2 numbers and stores both the result and the remainder.
///
/// @param numerator Signed integer to divide.
/// @param divisor Signed integer to divide by.
/// @param result Pointer to integer set to numerator / divisor.
/// @param remainder Pointer to integer set to numerator % divisor.
#ifdef __clang__
void swiDivMod(int numerator, int divisor, int *result, int *remainder);
#else
__attribute__((always_inline))
static inline void swiDivMod(int numerator, int divisor, int *result, int *remainder)
{
    register int r0 asm("r0") = numerator;
    register int r1 asm("r1") = divisor;
    asm volatile inline ("swi 0x9 << ((1f - . == 4) * -16); 1:" : "+r"(r0), "+r"(r1) :: "r3");
    *result = r0;
    *remainder = r1;
}
#endif

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
#ifdef __clang__
void swiCopy(const void *source, void *dest, int flags);
#else
__attribute__((always_inline))
static inline void swiCopy(const void *source, void *dest, int flags)
{
    register const void *r0 asm("r0") = source;
    register void *r1 asm("r1") = dest;
    register int r2 asm("r2") = flags;
    asm volatile inline ("swi 0xB << ((1f - . == 4) * -16); 1:" : "+r"(r0), "+r"(r1), "+r"(r2) :: "r3", "memory");
}
#endif

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
#ifdef __clang__
void swiFastCopy(const void *source, void *dest, int flags);
#else
__attribute__((always_inline))
static inline void swiFastCopy(const void *source, void *dest, int flags)
{
    register const void *r0 asm("r0") = source;
    register void *r1 asm("r1") = dest;
    register int r2 asm("r2") = flags;
    asm volatile inline ("swi 0xC << ((1f - . == 4) * -16); 1:" : "+r"(r0), "+r"(r1), "+r"(r2) :: "r3", "memory");
}
#endif

/// Calculates the square root.
///
/// @param value The value to calculate.
/// @return The square root of the value as an integer.
/// @note Use fixed point math if you want more accuracy.
#ifdef __clang__
int swiSqrt(int value);
#else
__attribute__((always_inline))
static inline int swiSqrt(int value)
{
    register int r0 asm("r0") = value;
    asm volatile inline ("swi 0xD << ((1f - . == 4) * -16); 1:" : "+r"(r0) :: "r1", "r3");
    return r0;
}
#endif

/// Calculates a CRC-16 checksum using the following configuration:
///
/// - Input reflected: Yes
/// - Result reflected: Yes
/// - Polynomial: 0x8005
///
/// @param crc Initial CRC-16 value.
/// @param data Pointer to data (processed nibble by nibble)
/// @param size Size in bytes.
/// @return The CRC-16 value after the data has been processed.
#ifdef __clang__
uint16_t swiCRC16(uint16_t crc, const void *data, uint32_t size);
#else
__attribute__((always_inline))
static inline uint16_t swiCRC16(uint16_t crc, const void *data, uint32_t size)
{
    register uint32_t r0 asm("r0") = crc;
    register const void *r1 asm("r1") = data;
    register uint32_t r2 asm("r2") = size;
    asm volatile inline ("swi 0xE << ((1f - . == 4) * -16); 1:" : "+r"(r0), "+r"(r1), "+r"(r2) :: "r3");
    return r0;
}
#endif

/// Returns 1 if running on a Nintendo hardware debugger.
///
/// @note It only works in DS mode (not DSi mode), and with the cache disabled.
/// It's recommended to use isHwDebugger() instead, which works in DSi mode too,
/// and it doesn't require the cache to be disabled.
///
/// @return 1 if running on a debugger (8 MB of RAM instead of 4 MB), else 0.
#ifdef __clang__
int swiIsDebugger(void);
#else
__attribute__((always_inline))
static inline int swiIsDebugger(void)
{
    register int i asm("r0");
    asm volatile inline ("swi 0xF << ((1f - . == 4) * -16); 1:" : "=r"(i) :: "r1", "r3");
    return i;
}
#endif

/// Unpack data stored in multiple elements in a byte to a larger space.
///
/// i.e. 8 elements per byte (i.e. b/w font), into 1 element per byte.
///
/// @param source Source address.
/// @param destination Destination address (word aligned).
/// @param params Pointer to an UnpackStruct.
#ifdef __clang__
void swiUnpackBits(const void *source, void *destination, TUnpackStruct *params);
#else
__attribute__((always_inline))
static inline void swiUnpackBits(const void *source, void *destination, TUnpackStruct *params)
{
    register const void* r0 asm("r0") = source;
    register void* r1 asm("r1") = destination;
    register const TUnpackStruct* r2 asm("r2") = params;
    asm volatile inline ("swi 0x10 << ((1f - . == 4) * -16); 1:" : "+r"(r0), "+r"(r1), "+r"(r2) :: "r3", "memory");
}
#endif

/// Decompresses LZSS compressed data.
///
/// @param source Pointer to a header word, followed by compressed data. bit 0-7
///               of header is ignored. bit 8-31 of header is size of
///               uncompressed data in bytes.
/// @param destination Destination address.
/// @note Writes data a byte at a time.
/// @see decompress.h
#ifdef __clang__
void swiDecompressLZSSWram(const void *source, void *destination);
#else
__attribute__((always_inline))
static inline void swiDecompressLZSSWram(const void *source, void *destination)
{
    register const void* r0 asm("r0") = source;
    register void* r1 asm("r1") = destination;
    asm volatile inline ("swi 0x11 << ((1f - . == 4) * -16); 1:" : "+r"(r0), "+r"(r1) :: "r3", "memory");
}
#endif

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
int swiDecompressLZSSVram(const void *source, void *destination, uint32_t toGetSize,
                          TDecompressionStream *stream);

#ifdef __clang__
int swiDecompressLZSSVramNTR(const void *source, void *destination, uint32_t toGetSize,
                             TDecompressionStream *stream);
#else
__attribute__((always_inline))
static inline int swiDecompressLZSSVramNTR(const void *source, void *destination, uint32_t toGetSize,
                             TDecompressionStream *stream)
{
    register int r0 asm("r0") = (int) source;
    register void* r1 asm("r1") = destination;
    register uint32_t r2 asm("r2") = toGetSize;
    register TDecompressionStream *r3 asm("r3") = stream;
    asm volatile inline ("swi 0x12 << ((1f - . == 4) * -16); 1:" : "+r"(r0), "+r"(r1), "+r"(r2), "+r"(r3) :: "memory");
    return r0;
}
#endif

#ifdef __clang__
int swiDecompressLZSSVramTWL(const void *source, void *destination, uint32_t toGetSize,
                             TDecompressionStream *stream);
#else
__attribute__((always_inline))
static inline int swiDecompressLZSSVramTWL(const void *source, void *destination, uint32_t toGetSize,
                             TDecompressionStream *stream)
{
    register int r0 asm("r0") = (int) source;
    register void* r1 asm("r1") = destination;
    register uint32_t r2 asm("r2") = toGetSize;
    register TDecompressionStream *r3 asm("r3") = stream;
    asm volatile inline ("swi 0x02 << ((1f - . == 4) * -16); 1:" : "+r"(r0), "+r"(r1), "+r"(r2), "+r"(r3) :: "memory");
    return r0;
}
#endif

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
#ifdef __clang__
int swiDecompressHuffman(const void *source, void *destination, uint32_t toGetSize,
                         TDecompressionStream *stream);
#else
__attribute__((always_inline))
static inline int swiDecompressHuffman(const void *source, void *destination, uint32_t toGetSize,
                         TDecompressionStream *stream)
{
    register int r0 asm("r0") = (int) source;
    register void* r1 asm("r1") = destination;
    register uint32_t r2 asm("r2") = toGetSize;
    register TDecompressionStream *r3 asm("r3") = stream;
    asm volatile inline ("swi 0x13 << ((1f - . == 4) * -16); 1:" : "+r"(r0), "+r"(r1), "+r"(r2), "+r"(r3) :: "memory");
    return r0;
}
#endif

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
#ifdef __clang__
void swiDecompressRLEWram(const void *source, void *destination);
#else
__attribute__((always_inline))
static inline void swiDecompressRLEWram(const void *source, void *destination)
{
    register const void* r0 asm("r0") = source;
    register void* r1 asm("r1") = destination;
    asm volatile inline ("swi 0x14 << ((1f - . == 4) * -16); 1:" : "+r"(r0), "+r"(r1) :: "r3", "memory");
}
#endif

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
#ifdef __clang__
int swiDecompressRLEVram(const void *source, void *destination, uint32_t toGetSize,
                         TDecompressionStream *stream);
#else
__attribute__((always_inline))
static inline int swiDecompressRLEVram(const void *source, void *destination, uint32_t toGetSize,
                         TDecompressionStream *stream)
{
    register int r0 asm("r0") = (int) source;
    register void* r1 asm("r1") = destination;
    register uint32_t r2 asm("r2") = toGetSize;
    register TDecompressionStream *r3 asm("r3") = stream;
    asm volatile inline ("swi 0x15 << ((1f - . == 4) * -16); 1:" : "+r"(r0), "+r"(r1), "+r"(r2), "+r"(r3) :: "memory");
    return r0;
}
#endif

#ifdef ARM9

/// Wait for any interrupt.
///
/// @note ARM9 exclusive.
#ifdef __clang__
void swiWaitForIRQ(void);
#else
__attribute__((always_inline))
static inline void swiWaitForIRQ(void)
{
    asm volatile inline ("swi 0x6 << ((1f - . == 4) * -16); 1:");
}
#endif

/// Writes a word of the data to 0x04000300:32
///
/// @param data The word to write.
/// @note This is on the ARM9, but works differently then the ARM7 function!
void swiSetHaltCR(uint32_t data);

/// Decodes a stream of bytes based on the difference of the bytes.
///
/// @param source Pointer to a header word, followed by encoded data.
///               word(31..8) = size of data (in bytes).
///               word(7..0) = ignored.
/// @param destination Destination address.
/// @note Writes data a byte at a time.
/// @note ARM9 exclusive.
#ifdef __clang__
void swiDecodeDelta8(const void *source, void *destination);
#else
__attribute__((always_inline))
static inline void swiDecodeDelta8(const void *source, void *destination)
{
    register const void* r0 asm("r0") = source;
    register void* r1 asm("r1") = destination;
    asm volatile inline ("swi 0x16 << ((1f - . == 4) * -16); 1:" : "+r"(r0), "+r"(r1) :: "r3", "memory");
}
#endif

/// Decodes a stream of bytes based on the difference of the bytes.
///
/// @param source Pointer to a header word, followed by encoded data.
///               word(31..8) = size of data (in bytes).
///               word(7..0) = ignored.
/// @param destination Destination address.
/// @note Writes data a halfword at a time.
/// @note ARM9 exclusive.
#ifdef __clang__
void swiDecodeDelta16(const void *source, void *destination);
#else
__attribute__((always_inline))
static inline void swiDecodeDelta16(const void *source, void *destination)
{
    register const void* r0 asm("r0") = source;
    register void* r1 asm("r1") = destination;
    asm volatile inline ("swi 0x18 << ((1f - . == 4) * -16); 1:" : "+r"(r0), "+r"(r1) :: "r3", "memory");
}
#endif

#endif

#ifdef ARM7

/// Writes a byte of the data to 0x04000301:8
///
/// @param data The byte to write.
/// @note ARM7 exclusive.
void swiSetHaltCR(uint8_t data);

/// Halts the CPU until an interupt occures.
///
/// @note ARM7 exclusive.
#ifdef __clang__
void swiHalt(void);
#else
__attribute__((always_inline))
static inline void swiHalt(void)
{
    asm volatile inline ("swi 0x6 << ((1f - . == 4) * -16); 1:");
}
#endif

/// Halts the CPU and most of the hardware untill an interupt occures.
///
/// @note ARM7 exclusive.
#ifdef __clang__
void swiSleep(void);
#else
__attribute__((always_inline))
static inline void swiSleep(void)
{
    asm volatile inline ("swi 0x7 << ((1f - . == 4) * -16); 1:");
}
#endif

/// Switches the DS to GBA mode.
///
/// @note ARM7 exclusive.
__attribute__((noreturn))
void swiSwitchToGBAMode(void);

/// Returns an entry in the sine table.
///
/// @param index The index of the sine table (0 - 63).
/// @return The entry.
/// @note ARM7 exclusive.
#ifdef __clang__
uint16_t swiGetSineTable(int index);
#else
__attribute__((always_inline))
static inline uint16_t swiGetSineTable(int index)
{
    register int r0 asm("r0") = index;
    asm volatile inline ("swi 0x1A << ((1f - . == 4) * -16); 1:" : "+r"(r0) :: "r1", "r3");
    return r0;
}
#endif

/// Returns an entry in the pitch table.
///
/// @param index The index of the pitch table (0 - 767).
/// @return The entry.
/// @note ARM7 exclusive.
#ifdef __clang__
uint16_t swiGetPitchTable(int index);
#else
__attribute__((always_inline))
static inline uint16_t swiGetPitchTable(int index)
{
    register int r0 asm("r0") = index;
    asm volatile inline ("swi 0x1B << ((1f - . == 4) * -16); 1:" : "+r"(r0) :: "r1", "r3");
    return r0;
}
#endif

/// Returns an entry in the volume table.
///
/// @param index The index of the volume table (0 - 723).
/// @return The entry.
/// @note ARM7 exclusive.
#ifdef __clang__
uint8_t swiGetVolumeTable(int index);
#else
__attribute__((always_inline))
static inline uint8_t swiGetVolumeTable(int index)
{
    register int r0 asm("r0") = index;
    asm volatile inline ("swi 0x1C << ((1f - . == 4) * -16); 1:" : "+r"(r0) :: "r1", "r3");
    return r0;
}
#endif

/// Increments or decrements the sound bias once per delay.
///
/// @param enabled Set to 0 to decrement it until it reaches 0x000, set to 1 to
///                increment it until it reaches 0x200.
/// @param delay Is in the same units of time as swiDelay.
/// @note ARM7 exclusive.
#ifdef __clang__
void swiChangeSoundBias(int enabled, int delay);
#else
__attribute__((always_inline))
static inline void swiChangeSoundBias(int enabled, int delay)
{
    register int r0 asm("r0") = enabled;
    register int r1 asm("r1") = delay;
    asm volatile inline ("swi 0x08 << ((1f - . == 4) * -16); 1:" : "+r"(r0), "+r"(r1) :: "r3");
}
#endif

#endif // ARM7

#ifdef __cplusplus
}
#endif

#endif // LIBNDS_NDS_BIOS_H__
