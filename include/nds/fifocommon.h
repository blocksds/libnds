// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (c) 2008-2015 Dave Murphy (WinterMute)
// Copyright (c) 2023 Antonio Niño Díaz

#ifndef LIBNDS_NDS_FIFOCOMMON_H__
#define LIBNDS_NDS_FIFOCOMMON_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <nds/cothread.h>
#include <nds/interrupts.h>
#include <nds/ndstypes.h>

/// @file nds/fifocommon.h
///
/// @brief Low level FIFO API.

/// Enum values for the different FIFO channels.
typedef enum {
    FIFO_PM         = 0,  ///< Channel used for power management
    FIFO_SOUND      = 1,  ///< Channel used for sound access
    FIFO_SYSTEM     = 2,  ///< Channel used for system functions
    FIFO_MAXMOD     = 3,  ///< Channel used for the maxmod library
    FIFO_DSWIFI     = 4,  ///< Channel used for the dswifi library
    FIFO_STORAGE    = 5,  ///< Channel used for DS cart, DLDI, DSi SD and NAND access
    FIFO_FIRMWARE   = 6,  ///< Channel used for firmware access
    FIFO_CAMERA     = 7,  ///< Channel used for camera access
    FIFO_USER_01    = 8,  ///< Channel available for users
    FIFO_USER_02    = 9,  ///< Channel available for users
    FIFO_USER_03    = 10, ///< Channel available for users
    FIFO_USER_04    = 11, ///< Channel available for users
    FIFO_USER_05    = 12, ///< Channel available for users
    FIFO_USER_06    = 13, ///< Channel available for users
    FIFO_USER_07    = 14, ///< Channel available for users
    FIFO_USER_08    = 15, ///< Channel available for users

    FIFO_SDMMC      = 5,  ///< Deprecated name of FIFO_STORAGE
} FifoChannels;

/// Enum values for the FIFO sound commands.
typedef enum {
    SOUND_SET_PAN           = 0 << 20,
    SOUND_SET_VOLUME        = 1 << 20,
    SOUND_SET_FREQ          = 2 << 20,
    SOUND_SET_WAVEDUTY      = 3 << 20,
    SOUND_MASTER_ENABLE     = 4 << 20,
    SOUND_MASTER_DISABLE    = 5 << 20,
    SOUND_PAUSE             = 6 << 20,
    SOUND_RESUME            = 7 << 20,
    SOUND_KILL              = 8 << 20,
    SOUND_SET_MASTER_VOL    = 9 << 20,
    MIC_STOP                = 10 << 20
} FifoSoundCommand;

/// Enum values for the FIFO system commands.
typedef enum {
    SYS_REQ_TOUCH,
    SYS_REQ_KEYS,
    SYS_REQ_TIME,
    SYS_SET_TIME,
    SDMMC_INSERT,
    SDMMC_REMOVE
} FifoSystemCommands;

typedef enum {
    SDMMC_SD_START,
    SDMMC_SD_STOP,
    SDMMC_SD_STATUS,
    SDMMC_SD_SIZE,
    SDMMC_NAND_START,
    SDMMC_NAND_STOP,
    SDMMC_NAND_STATUS,
    SDMMC_NAND_SIZE,
    DLDI_STARTUP,
    DLDI_IS_INSERTED,
    DLDI_READ_SECTORS,
    DLDI_WRITE_SECTORS,
    DLDI_CLEAR_STATUS,
    DLDI_SHUTDOWN,
} FifoSdmmcCommands;

typedef enum {
    FW_READ,
    FW_WRITE
} FifoFirmwareCommands;

/// Enum values for the FIFO power management commands.
typedef enum {
    PM_REQ_ON               = 1 << 16,
    PM_REQ_OFF              = 2 << 16,
    PM_REQ_LED              = 3 << 16,
    PM_REQ_SLEEP            = 4 << 16,
    PM_REQ_SLEEP_DISABLE    = 5 << 16,
    PM_REQ_SLEEP_ENABLE     = 6 << 16,
    PM_REQ_BATTERY          = 7 << 16,
    PM_REQ_SLOT1_DISABLE    = 8 << 16,
    PM_REQ_SLOT1_ENABLE     = 9 << 16,
} FifoPMCommands;

/// Enum values for the FIFO wifi commands.
typedef enum {
    WIFI_ENABLE,
    WIFI_DISABLE,
    WIFI_SYNC,
    WIFI_STARTUP
} FifoWifiCommands;


/// Power Management LED blink mode control bits.
typedef enum {
    PM_LED_ON       = 0, ///< Steady on
    PM_LED_SLEEP    = 1, ///< Blinking, mostly off
    PM_LED_BLINK    = 3, ///< Blinking, mostly on
} PM_LedBlinkMode;

/// Callback that is called with the address sent from the other CPU and the
/// callback's user data.
///
/// The handler is called when new data arrives.
///
/// @note Callback functions are called from an interrupt handler. Try to not
/// use too much stack from the callback.
typedef void (*FifoAddressHandlerFunc)(void *address, void *userdata);

/// Callback that is called with the 32-bit value sent from the other CPU and
/// the callback's user data.
///
/// The handler is called when new data arrives.
///
/// @note Callback functions are called from an interrupt handler. Try to not
/// use too much stack from the callback.
typedef void (*FifoValue32HandlerFunc)(u32 value32, void *userdata);

/// Callback that is called with the number of bytes sent from the other CPU and
/// the callback's user data.
///
/// The handler is called when new data arrives. This callback must call
/// fifoGetDatamsg() to actually retrieve the data. If it doesn't, the data will
/// be destroyed on return.
///
/// @note Callback functions are called from an interrupt handler. Try to not
/// use too much stack from the callback.
typedef void (*FifoDatamsgHandlerFunc)(int num_bytes, void *userdata);

/// Initializes the FIFO system.
///
/// Attempts to sync with the other CPU. If it fails, FIFO services won't be
/// provided.
///
/// @return Returns true on success, false on error.
///
/// @note call irqInit() before calling this function.
bool fifoInit(void);

/// Sends a main RAM address to the other CPU.
///
/// @param channel Channel number.
/// @param address Address to send (0x02000000-0x02FFFFFF).
///
/// @return Returns true if the data message has been sent, false on error.
bool fifoSendAddress(u32 channel, void *address);

/// Sends a 32-bit value to the other CPU.
///
/// @param channel Channel number.
/// @param value32 Value to send.
///
/// @return Returns true if the data message has been sent, false on error.
///
/// @note Sending a value with the top 8 bits set to zero is faster.
bool fifoSendValue32(u32 channel, u32 value32);

/// Sends a sequence of bytes to the other CPU.
///
/// @param channel Channel number.
/// @param num_bytes Number of bytes to send (0 to FIFO_MAX_DATA_BYTES).
/// @param data_array Pointer to data array
///
/// @return Returns true if the data message has been sent, false on error.
bool fifoSendDatamsg(u32 channel, u32 num_bytes, u8 * data_array);

/// Sends a special command to the other CPU.
///
/// @param cmd Command identifier.
///
/// @return Returns true if the message has been sent, false on error.
bool fifoSendSpecialCommand(u32 cmd);

/// Sets user address message callback.
///
/// Sets a callback to receive incoming address messages of a specific channel.
///
/// @param channel Channel number.
/// @param newhandler Function pointer to the new handler.
/// @param userdata Pointer that will be passed on to the handler when it will
/// be called (as "userdata").
///
/// @return Returns true if the handler has been set, false on error.
///
/// @note Setting the handler for a channel feeds the queue of buffered messages
/// to the new handler if there are any unread messages.
bool fifoSetAddressHandler(u32 channel, FifoAddressHandlerFunc newhandler, void * userdata);

/// Sets user value32 message callback.
///
/// Sets a callback to receive incoming value32 messages of a specific channel.
///
/// @param channel Channel number.
/// @param newhandler Function pointer to the new handler.
/// @param userdata Pointer that will be passed on to the handler when it will
/// be called (as "userdata").
///
/// @return Returns true if the handler has been set, false on error.
///
/// @note Setting the handler for a channel feeds the queue of buffered messages
/// to the new handler if there are any unread messages.
bool fifoSetValue32Handler(u32 channel, FifoValue32HandlerFunc newhandler, void *userdata);

/// Sets user data message callback.
///
/// Sets a callback to receive incoming data messages of a specific channel.
///
/// @param channel Channel number.
/// @param newhandler Function pointer to the new handler.
/// @param userdata Pointer that will be passed on to the handler when it will
/// be called (as "userdata").
///
/// @return Returns true if the handler has been set, false on error.
///
/// @note Setting the handler for a channel feeds the queue of buffered messages
/// to the new handler if there are any unread messages.
bool fifoSetDatamsgHandler(u32 channel, FifoDatamsgHandlerFunc newhandler, void *userdata);

/// Checks if there are any address messages in the queue.
///
/// @param channel Channel number.
///
/// @return Returns true if there are messages in the queue and there isn't a
/// message handler in place for the channel.
bool fifoCheckAddress(u32 channel);

/// Checks if there are any value32 messages in the queue.
///
/// @param channel Channel number.
///
/// @return Returns true if there are messages in the queue and there isn't a
/// message handler in place for the channel.
bool fifoCheckValue32(u32 channel);

/// Checks if there are any data messages in the queue.
///
/// @param channel Channel number.
///
/// @return Returns true if there are messages in the queue and there isn't a
/// message handler in place for the channel.
bool fifoCheckDatamsg(u32 channel);

/// Gets the size of the first message in the queue of a specific channel.
///
/// @param channel Channel number.
///
/// @return Returns the number of bytes of the first message in the queue, -1 if
/// there are no messages.
int fifoCheckDatamsgLength(u32 channel);

/// Gets the first address in the queue of a specific channel.
///
/// @param channel Channel number.
///
/// @return Returns the first address in the queue, or NULL if there is none.
void *fifoGetAddress(u32 channel);

/// Gets the first value32 in the queue of a specific channel.
///
/// @param channel Channel number.
///
/// @return Returns the first value32 in the queue, or 0 if there is no message.
u32 fifoGetValue32(u32 channel);

/// Reads a data message in a given buffer.
///
/// @param channel Channel number.
/// @param buffersize Size of the provided buffer.
/// @param destbuffer Pointer to the buffer to store the message.
///
/// @return The number of bytes written, or -1 if there is no message.
/// @warning If your buffer is not big enough, you may lose data. Check the
/// actual size first with fifoCheckDatamsgLength().
int fifoGetDatamsg(u32 channel, int buffersize, u8 *destbuffer);

/// Waits for any value32 message in a FIFO channel and blocks until there is
/// one available.
///
/// @param channel Channel number.
static inline void fifoWaitValue32(u32 channel)
{
    while (!fifoCheckValue32(channel))
        swiIntrWait(1, IRQ_FIFO_NOT_EMPTY);
}

/// Waits for any value32 message in a FIFO channel and yields until there isn't
/// one available.
///
/// @param channel Channel number.
static inline void fifoWaitValue32Async(u32 channel)
{
    while (!fifoCheckValue32(channel))
    {
#ifdef ARM9
        cothread_yield_irq(IRQ_FIFO_NOT_EMPTY);
#else
        swiIntrWait(1, IRQ_FIFO_NOT_EMPTY);
#endif
    }
}

/// Waits for any address message in a FIFO channel and blocks until there is
/// one available.
///
/// @param channel Channel number.
static inline void fifoWaitAddress(u32 channel)
{
    while (!fifoCheckAddress(channel))
        swiIntrWait(1, IRQ_FIFO_NOT_EMPTY);
}

/// Waits for any address message in a FIFO channel and yields until there isn't
/// one available.
///
/// @param channel Channel number.
static inline void fifoWaitAddressAsync(u32 channel)
{
    while (!fifoCheckAddress(channel))
    {
#ifdef ARM9
        cothread_yield_irq(IRQ_FIFO_NOT_EMPTY);
#else
        swiIntrWait(1, IRQ_FIFO_NOT_EMPTY);
#endif
    }
}

/// Waits for any data message in a FIFO channel and blocks until there is
/// one available.
///
/// @param channel Channel number.
static inline void fifoWaitDatamsg(u32 channel)
{
    while (!fifoCheckDatamsg(channel))
        swiIntrWait(1, IRQ_FIFO_NOT_EMPTY);
}

/// Waits for any data message in a FIFO channel and yields until there isn't
/// one available.
///
/// @param channel Channel number.
static inline void fifoWaitDatamsgAsync(u32 channel)
{
    while (!fifoCheckDatamsg(channel))
    {
#ifdef ARM9
        cothread_yield_irq(IRQ_FIFO_NOT_EMPTY);
#else
        swiIntrWait(1, IRQ_FIFO_NOT_EMPTY);
#endif
    }
}

#ifdef ARM9

/// Acquires the mutex of the specified FIFO channel.
///
/// @param channel Channel number.
void fifoMutexAcquire(u32 channel);

/// Tries to acquire the mutex of the specified FIFO channel.
///
/// @return Returns true if the lock was acquired, false if not.
/// @param channel Channel number.
bool fifoMutexTryAcquire(u32 channel);

/// Releases the mutex of the specified FIFO channel.
///
/// @param channel Channel number.
void fifoMutexRelease(u32 channel);

#endif // ARM9

#ifdef __cplusplus
};
#endif

#endif // LIBNDS_NDS_FIFOCOMMON_H__
