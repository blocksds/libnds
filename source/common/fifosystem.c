// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2008-2015 Dave Murphy (WinterMute)
// Copyright (C) 2023-2025 Antonio Niño Díaz

#include <stdlib.h>
#include <string.h>

#include <nds/bios.h>
#include <nds/cothread.h>
#include <nds/exceptions.h>
#include <nds/fifocommon.h>
#include <nds/interrupts.h>
#include <nds/ipc.h>
#include <nds/system.h>

#include "fifo_messages_helpers.h"

// Arbitrary maximum number of bytes that can be sent in a fifo data message.
// In practice, the maximum number of bytes that could fit is around
// FIFO_BUFFER_ENTRIES * 4 bytes per entry, but that would fill all the FIFO
// buffer with only one message.
#define FIFO_MAX_DATA_BYTES     128

// Number of words that can be stored temporarily while waiting to deque them
#ifdef ARM9
#define FIFO_BUFFER_ENTRIES     256
#else // ARM7
#define FIFO_BUFFER_ENTRIES     256
#endif

// The memory overhead of this library (per CPU) is:
//
//     16 + (NUM_CHANNELS * 32) + (FIFO_BUFFER_ENTRIES * 8)
//
// For 16 channels and 256 entries, this is 16 + 512 + 2048 = 2576 bytes of ram.
//
// Some padding may be added by the compiler, though.

// This value is used in the "next" field of a block to mean that there are no
// more entries in the queue.
#define FIFO_BUFFER_TERMINATE   0xFFFF

// Global FIFO pool
// ----------------

typedef struct
{
    // Index of next block in the list. If it's equal to FIFO_BUFFER_TERMINATE"
    // it means that this is the end of the list.
    u16 next;

    // Used for data messages. Size of the message in bytes.
    u16 extra;

    // Useful data kept in this entry.
    u32 data;
}
PACKED global_fifo_pool_entry;

// This pool of blocks stores all information regarding FIFO packets. It
// allocates a fixed amount of space that holds all packets waiting to be sent
// to the other CPU as well as packets that have been received but not handled.
static global_fifo_pool_entry global_fifo_pool[FIFO_BUFFER_ENTRIES];

// This variable is used as a shortcut to check if a message fits in the FIFO
// pool or not (rather than having to iterate through the queue of free blocks,
// which would take far longer).
static vu32 global_pool_free_words;

// Helpers to access fields of global_fifo_pool
#define POOL_DATA(index) global_fifo_pool[index].data
#define POOL_NEXT(index) global_fifo_pool[index].next
#define POOL_EXTRA(index) global_fifo_pool[index].extra

// FIFO queues
// -----------

// This represents a queue of blocks inside the global FIFO pool. The head
// points to a block that will point to another block, and so on, until it
// reaches the tail specified in this struct.
typedef struct fifo_queue
{
    vu16 head;
    vu16 tail;
}
fifo_queue;

// Queues that hold received address, data and value32 messages for each channel
static fifo_queue fifo_address_queue[FIFO_NUM_CHANNELS];
static fifo_queue fifo_data_queue[FIFO_NUM_CHANNELS];
static fifo_queue fifo_value32_queue[FIFO_NUM_CHANNELS];

// Queue that holds all free blocks.
static fifo_queue fifo_free_queue;

// Queues that hold the blocks to be sent and received.
static fifo_queue fifo_tx_queue;
static fifo_queue fifo_rx_queue;

// Helpers to allocate and free blocks in the global pool
// ------------------------------------------------------

// Try to allocate a new block. If it fails, it returns FIFO_BUFFER_TERMINATE.
// If not, it returns the index of the block it has just allocated.
static u32 fifo_buffer_alloc_block(void)
{
    if (global_pool_free_words == 0)
        return FIFO_BUFFER_TERMINATE;

    global_pool_free_words--;

    // Return the first entry in the free blocks queue
    u32 entry = fifo_free_queue.head;

    // We're going to use the first entry in the queue for the new block, so
    // move the head of the free blocks queue to the next entry in the queue.
    fifo_free_queue.head = POOL_NEXT(entry);

    // This function can't recreate the queue from scratch if its last entry
    // disappears. global_pool_free_words should ensure that this never
    // happens, but this assert() is here to double-check that in debug builds.
    assert(entry != FIFO_BUFFER_TERMINATE);

    // The newly allocated block will be added to the end of some other queue,
    // so mark it as the end of a queue.
    POOL_NEXT(entry) = FIFO_BUFFER_TERMINATE;

    return entry;
}

// Allocate a new block, blocking until there is an available slot.
static u32 fifo_buffer_wait_block(void)
{
    while (1)
    {
        u32 block = fifo_buffer_alloc_block();

        if (block != FIFO_BUFFER_TERMINATE)
            return block;

        // There are no free blocks. We need to wait until the other CPU
        // receives some words and we can free up some space in our TX buffer.
        // TODO: This waits until all of the hardware TX FIFO has been emptied.
        // It may be better to wait until it isn't full.
        // TODO: Enabling interrupts may be dangerous, this needs to be
        // double-checked.
        REG_IPC_FIFO_CR |= IPC_FIFO_SEND_EMPTY_IRQ;
        REG_IME = 1;
        swiIntrWait(INTRWAIT_KEEP_FLAGS, IRQ_SEND_FIFO);
        REG_IME = 0;
    }
}

// Frees the specified block.
static void fifo_buffer_free_block(u32 index)
{
    // Mark this block as the end of the queue
    POOL_NEXT(index) = FIFO_BUFFER_TERMINATE;
    POOL_EXTRA(index) = 0;

    // Make the previous end of the queue point to the new end of the queue
    POOL_NEXT(fifo_free_queue.tail) = index;

    // Update pointer to the end of the queue
    fifo_free_queue.tail = index;

    global_pool_free_words++;
}

// Adds a list of blocks from the FIFO buffer to a queue.
static void fifo_queue_append_list(fifo_queue *queue, int head, int tail)
{
    // Mark the end of the provided list as the end of the queue
    POOL_NEXT(tail) = FIFO_BUFFER_TERMINATE;

    if (queue->head == FIFO_BUFFER_TERMINATE)
    {
        // If the FIFO queue is empty, create it from scratch
        queue->head = head;
        queue->tail = tail;
    }
    else
    {
        // If the FIFO queue wasn't empty, make the old tail point to the
        // user-provided head.
        POOL_NEXT(queue->tail) = head;

        // Update pointer to the end of the user-provided queue
        queue->tail = tail;
    }
}

// Adds a block from the FIFO buffer to a queue.
static void fifo_queue_append_block(fifo_queue *queue, int block)
{
    fifo_queue_append_list(queue, block, block);
}

// Per-channel callbacks to handle received messages
// -------------------------------------------------

// Callbacks to be called whenever there is a new message
static FifoAddressHandlerFunc fifo_address_func[FIFO_NUM_CHANNELS];
static FifoValue32HandlerFunc fifo_value32_func[FIFO_NUM_CHANNELS];
static FifoDatamsgHandlerFunc fifo_datamsg_func[FIFO_NUM_CHANNELS];

// User data to be passed to the callbacks in the last argument.
static void *fifo_address_data[FIFO_NUM_CHANNELS];
static void *fifo_value32_data[FIFO_NUM_CHANNELS];
static void *fifo_datamsg_data[FIFO_NUM_CHANNELS];

// Set a callback to receive incoming address messages on a specific channel.
bool fifoSetAddressHandler(u32 channel, FifoAddressHandlerFunc newhandler, void *userdata)
{
    if (channel >= FIFO_NUM_CHANNELS)
        return false;

    int oldIME = enterCriticalSection();

    fifo_address_func[channel] = newhandler;
    fifo_address_data[channel] = userdata;

    // If a new handler has been set, check if there are pending messages
    // and send them right away.
    if (newhandler)
    {
        while (fifoCheckAddress(channel))
        {
            newhandler(fifoGetAddress(channel), userdata);
        }
    }

    leaveCriticalSection(oldIME);

    return true;
}

// Set a callback to receive incoming value32 messages on a specific channel.
bool fifoSetValue32Handler(u32 channel, FifoValue32HandlerFunc newhandler, void *userdata)
{
    if (channel >= FIFO_NUM_CHANNELS)
        return false;

    int oldIME = enterCriticalSection();

    fifo_value32_func[channel] = newhandler;
    fifo_value32_data[channel] = userdata;

    // If a new handler has been set, check if there are pending messages
    // and send them right away.
    if (newhandler)
    {
        while (fifoCheckValue32(channel))
        {
            newhandler(fifoGetValue32(channel), userdata);
        }
    }

    leaveCriticalSection(oldIME);

    return true;
}

// Set a callback to receive incoming data sequences on a specific channel.
bool fifoSetDatamsgHandler(u32 channel, FifoDatamsgHandlerFunc newhandler, void *userdata)
{
    if (channel >= FIFO_NUM_CHANNELS)
        return false;

    int oldIME = enterCriticalSection();

    fifo_datamsg_func[channel] = newhandler;
    fifo_datamsg_data[channel] = userdata;

    // If a new handler has been set, check if there are pending messages
    // and send them right away.
    if (newhandler)
    {
        while (fifoCheckDatamsg(channel))
        {
            int block = fifo_data_queue[channel].head;
            int n_bytes = fifo_msg_data_unpack_length(POOL_DATA(block));
            newhandler(n_bytes, userdata);

            // If the user hasn't fetched the message from the queue by calling
            // fifoGetDatamsg(), it is still in the queue. Delete it now.
            if (block == fifo_data_queue[channel].head)
                fifoGetDatamsg(channel, 0, 0);
        }
    }

    leaveCriticalSection(oldIME);

    return true;
}

// Hardware TX and RX queues handlers
// ----------------------------------

// Fills the hardware TX FIFO with as many words from the software TX queue as
// we can fit.
//
// If there are too many words to be sent and some remain pending, enable an
// interrupt that will be triggered when all the words in the TX hardware
// registers are received by the other CPU.
//
// If all words fit in the hardware TX registers, disable that IRQ.
static void fifoFillTxFifoFromBuffer(void)
{
    u32 head = fifo_tx_queue.head;

    while (1)
    {
        // We have reached the end of the words to send. Disable the IRQ.
        if (head == FIFO_BUFFER_TERMINATE)
        {
            REG_IPC_FIFO_CR &= ~IPC_FIFO_SEND_EMPTY_IRQ;
            break;
        }

        // The TX FIFO is full, enable the IRQ.
        if (REG_IPC_FIFO_CR & IPC_FIFO_SEND_FULL)
        {
            REG_IPC_FIFO_CR |= IPC_FIFO_SEND_EMPTY_IRQ;
            break;
        }

        u32 next = POOL_NEXT(head);

        REG_IPC_FIFO_TX = POOL_DATA(head);

        fifo_buffer_free_block(head);
        head = next;
    }

    fifo_tx_queue.head = head;
}

// Get all available entries from the hardware RX FIFO and save them in the
// software RX queue for processing.
static void fifoFillBufferFromRxFifo(void)
{
    while (1)
    {
        if (REG_IPC_FIFO_CR & IPC_FIFO_RECV_EMPTY)
            break;

        u32 block = fifo_buffer_alloc_block();

        // There is no more space in global_fifo_pool, stop saving blocks until
        // some of them get processed.
        if (block == FIFO_BUFFER_TERMINATE)
            break;

        POOL_DATA(block) = REG_IPC_FIFO_RX;

        fifo_queue_append_block(&fifo_rx_queue, block);
    }
}

static void fifoProcessRxBuffer(void)
{
    while (fifo_rx_queue.head != FIFO_BUFFER_TERMINATE)
    {
        u32 block = fifo_rx_queue.head;
        u32 data = POOL_DATA(block);

        u32 channel = fifo_msg_unpack_channel(data);

        if (fifo_msg_type_is_special_command(data))
        {
            uint32_t cmd = data & FIFO_SPECIAL_COMMAND_MASK;
#ifdef ARM9
            // Message sent from the ARM7 to the ARM9 to start a reset
            if (cmd == FIFO_ARM7_REQUESTS_ARM9_RESET)
            {
                REG_IME = 1;
                exit(0);
            }
#endif

#ifdef ARM7
            // Message sent from the ARM9 to the ARM7 to start a reset
            if (cmd == FIFO_ARM9_REQUESTS_ARM7_RESET)
            {
                REG_IME = 1;
                // Make sure that the two CPUs reset at the same time. The other
                // CPU reset function (located in the bootstub struct) is
                // responsible for issuing the same commands to ensure that both
                // CPUs are in sync and they reset at the same time.
                REG_IPC_SYNC = 0x100;
                while ((REG_IPC_SYNC & 0x0f) != 1);
                REG_IPC_SYNC = 0;
                swiSoftReset();
            }
#endif

            // Special commands are supposed to be used internally by libnds.
            // Receiving an unknown command is a fatal error.
            libndsCrash("Unknown FIFO command");
        }
        else if (fifo_msg_type_is_address(data))
        {
            void *address = fifo_msg_address_unpack(data);

            fifo_rx_queue.head = POOL_NEXT(block);
            if (fifo_address_func[channel])
            {
                fifo_buffer_free_block(block);
                REG_IME = 1;
                fifo_address_func[channel](address, fifo_address_data[channel]);
                REG_IME = 0;
            }
            else
            {
                POOL_DATA(block) = (u32)address;
                fifo_queue_append_block(&fifo_address_queue[channel], block);
            }
        }
        else if (fifo_msg_type_is_value32(data))
        {
            u32 value32;

            if (fifo_msg_value32_has_extra(data))
            {
                int next = POOL_NEXT(block);

                // If the extra word hasn't been received, try later
                if (next == FIFO_BUFFER_TERMINATE)
                    break;

                fifo_buffer_free_block(block);
                block = next;
                value32 = POOL_DATA(block);
            }
            else
            {
                value32 = fifo_msg_value32_unpack_noextra(data);
            }

            // Increase read pointer
            fifo_rx_queue.head = POOL_NEXT(block);

            if (fifo_value32_func[channel])
            {
                fifo_buffer_free_block(block);
                REG_IME = 1;
                fifo_value32_func[channel](value32, fifo_value32_data[channel]);
                REG_IME = 0;
            }
            else
            {
                POOL_DATA(block) = value32;
                fifo_queue_append_block(&fifo_value32_queue[channel], block);
            }
        }
        else if (fifo_msg_type_is_data(data))
        {
            // Calculate the number of expected blocks
            int n_bytes = fifo_msg_data_unpack_length(data);
            int n_words = (n_bytes + 3) >> 2;

            // Count the number of available blocks
            int count = 0;
            int end = block;
            while (count < n_words && POOL_NEXT(end) != FIFO_BUFFER_TERMINATE)
            {
                end = POOL_NEXT(end);
                count++;
            }

            // If we haven't received enough blocks, try later
            if (count != n_words)
                break;

            fifo_rx_queue.head = POOL_NEXT(end);

            // Add messages from the FIFO buffer to the RX queue.
            int tmp = POOL_NEXT(block);
            fifo_buffer_free_block(block);

            POOL_EXTRA(tmp) = n_bytes;

            fifo_queue_append_list(&fifo_data_queue[channel], tmp, end);
            if (fifo_datamsg_func[channel])
            {
                block = fifo_data_queue[channel].head;

                // Call the handler and tell it the number of available bytes to
                // use. They need to be fetched and turned into a proper message
                // by calling fifoGetDatamsg().
                REG_IME = 1;
                fifo_datamsg_func[channel](n_bytes, fifo_datamsg_data[channel]);
                REG_IME = 0;

                // If the user hasn't fetched the message from the queue by
                // calling fifoGetDatamsg(), it is still in the queue. Delete it
                // now.
                if (block == fifo_data_queue[channel].head)
                    fifoGetDatamsg(channel, 0, 0);
            }
        }
        else
        {
            fifo_rx_queue.head = POOL_NEXT(block);
            fifo_buffer_free_block(block);
        }
    }
}

static volatile bool fifo_rx_processing = false;

static void fifoReadRxFifoAndProcessBuffer(void)
{
    fifoFillBufferFromRxFifo();

    // This handler can be nested. This check makes sure that there is
    // only one level of nesting, and that the nested handler can only read data
    // from the IPC registers and save it to the FIFO RX queue. The processing
    // will happen in the non-nested handler when the nested handler finishes.
    if (fifo_rx_processing)
        return;

    fifo_rx_processing = true;

    fifoProcessRxBuffer();

    fifo_rx_processing = false;
}

// Helpers add messages to the software TX queue
// ---------------------------------------------

static bool fifoInternalSend(u32 firstword, u32 extrawordcount, u32 *wordlist)
{
    // If the caller has provided at least one extra word, check that the
    // pointer with data isn't NULL. If not, ignore both values.
    if ((extrawordcount > 0) && (wordlist == NULL))
        return false;

    if (extrawordcount > (FIFO_MAX_DATA_BYTES / 4))
        return false;

    int oldIME = enterCriticalSection();

    // Check if there's enough space to send the whole message. If not, try to
    // flush some words pending from the software queue into the hardware TX
    // queue. If that doesn't free up enough space, give up.
    if (global_pool_free_words < extrawordcount + 1)
    {
        fifoFillTxFifoFromBuffer();

        if (global_pool_free_words < extrawordcount + 1)
        {
            leaveCriticalSection(oldIME);
            return false;
        }
    }

    u32 head = fifo_buffer_wait_block();
    if (fifo_tx_queue.head == FIFO_BUFFER_TERMINATE)
    {
        fifo_tx_queue.head = head;
    }
    else
    {
        POOL_NEXT(fifo_tx_queue.tail) = head;
    }
    POOL_DATA(head) = firstword;
    fifo_tx_queue.tail = head;

    // Add the words we're trying to send to the software queue.

    u32 count = 0;
    while (count < extrawordcount)
    {
        // TODO: Try to write words directly in the hardware TX queue instead of
        // adding them to the software queue and from there to the hardware
        // queue.

        u32 next = fifo_buffer_wait_block();
        if (fifo_tx_queue.head == FIFO_BUFFER_TERMINATE)
        {
            fifo_tx_queue.head = next;
        }
        else
        {
            POOL_NEXT(fifo_tx_queue.tail) = next;
        }
        POOL_DATA(next) = wordlist[count];
        count++;
        fifo_tx_queue.tail = next;
    }

    // Start the transfer by adding some words from the software queue to the
    // hardware queue.
    fifoFillTxFifoFromBuffer();

    leaveCriticalSection(oldIME);

    return true;
}

// Send a special command to the other CPU
bool fifoSendSpecialCommand(u32 cmd)
{
    return fifoInternalSend(fifo_msg_special_command_pack(cmd), 0, 0);
}

// Send an address (from mainram only) to the other cpu (on a specific channel)
// Addresses can be in the range of 0x02000000-0x02FFFFFF
bool fifoSendAddress(u32 channel, void *address)
{
    if (channel >= FIFO_NUM_CHANNELS)
        return false;

    if (!fifo_msg_address_is_pointer_valid(address))
        return false;

    return fifoInternalSend(fifo_msg_address_pack(channel, address), 0, 0);
}

bool fifoSendValue32(u32 channel, u32 value32)
{
    if (channel >= FIFO_NUM_CHANNELS)
        return false;

    u32 send_first, send_extra[1];

    if (fifo_msg_value32_needs_extra(value32))
    {
        // The value doesn't fit in just one 32-bit message
        send_first = fifo_msg_value32_pack_extra(channel);
        send_extra[0] = value32;
        return fifoInternalSend(send_first, 1, send_extra);
    }
    else
    {
        // The value fits in a 32-bit message
        send_first = fifo_msg_value32_pack(channel, value32);
        return fifoInternalSend(send_first, 0, 0);
    }
}

bool fifoSendDatamsg(u32 channel, u32 num_bytes, u8 *data_array)
{
    if (channel >= FIFO_NUM_CHANNELS)
        return false;

    if (num_bytes == 0)
    {
        u32 send_first = fifo_msg_data_pack_header(channel, 0);
        return fifoInternalSend(send_first, 0, NULL);
    }

    if (data_array == NULL)
        return false;

    if (num_bytes > FIFO_MAX_DATA_BYTES)
        return false;

    u32 num_words = (num_bytes + 3) >> 2;

    // Early check. fifoInternalSend() will do another check, but this one will
    // save us time from preparing buffer_array[].
    if (global_pool_free_words < num_words + 1)
        return false;

    u32 buffer_array[num_words]; // TODO: This is a VLA, remove?
    // Clear the last few bytes before the copy. The rest of the array will be
    // overwritten by memcpy().
    buffer_array[num_words - 1] = 0;
    memcpy(buffer_array, data_array, num_bytes);

    u32 send_first = fifo_msg_data_pack_header(channel, num_bytes);

    return fifoInternalSend(send_first, num_words, buffer_array);
}

// Helpers to get messages from the software RX queues
// ---------------------------------------------------

void *fifoGetAddress(u32 channel)
{
    if (channel >= FIFO_NUM_CHANNELS)
        return NULL;

    int oldIME = enterCriticalSection();

    int block = fifo_address_queue[channel].head;
    if (block == FIFO_BUFFER_TERMINATE)
    {
        leaveCriticalSection(oldIME);
        return NULL;
    }

    void *address = (void *)POOL_DATA(block);
    fifo_address_queue[channel].head = POOL_NEXT(block);
    fifo_buffer_free_block(block);

    fifoReadRxFifoAndProcessBuffer();

    leaveCriticalSection(oldIME);
    return address;
}

u32 fifoGetValue32(u32 channel)
{
    if (channel >= FIFO_NUM_CHANNELS)
        return 0;

    int oldIME = enterCriticalSection();

    int block = fifo_value32_queue[channel].head;
    if (block == FIFO_BUFFER_TERMINATE)
    {
        leaveCriticalSection(oldIME);
        return 0;
    }

    u32 value32 = POOL_DATA(block);
    fifo_value32_queue[channel].head = POOL_NEXT(block);
    fifo_buffer_free_block(block);

    fifoReadRxFifoAndProcessBuffer();

    leaveCriticalSection(oldIME);
    return value32;
}

// This function gets a data message from the queue of a channel and saves it to
// the buffer provided by the user. If the buffer size is smaller than the
// message, the function copies as much data as possible and deletes the message
// from the queue. It is also possible to pass 0 as size to delete the message
// from the queue. Use fifoCheckDatamsgLength() to determine the size before
// calling fifoGetDatamsg().
int fifoGetDatamsg(u32 channel, int buffersize, u8 *destbuffer)
{
    if (channel >= FIFO_NUM_CHANNELS)
        return -1;

    int oldIME = enterCriticalSection();

    int block = fifo_data_queue[channel].head;
    if (block == FIFO_BUFFER_TERMINATE)
    {
        leaveCriticalSection(oldIME);
        return -1;
    }

    int num_bytes = POOL_EXTRA(block);
    int num_words = (num_bytes + 3) >> 2;

    int copied_bytes = 0;

    for (int i = 0; i < num_words; i++)
    {
        u32 data = POOL_DATA(block);

        for (int j = 0; j < 4; j++)
        {
            if (copied_bytes < buffersize)
            {
                *destbuffer++ = data & 0xFF;
                data = data >> 8;
                copied_bytes++;
            }
        }

        int next = POOL_NEXT(block);
        fifo_buffer_free_block(block);
        block = next;
        if (block == FIFO_BUFFER_TERMINATE)
            break;
    }
    fifo_data_queue[channel].head = block;

    fifoReadRxFifoAndProcessBuffer();

    leaveCriticalSection(oldIME);

    return copied_bytes;
}

bool fifoCheckAddress(u32 channel)
{
    if (channel >= FIFO_NUM_CHANNELS)
        return false;

    int oldIME = enterCriticalSection();
    fifoReadRxFifoAndProcessBuffer();
    leaveCriticalSection(oldIME);

    return fifo_address_queue[channel].head != FIFO_BUFFER_TERMINATE;
}

bool fifoCheckDatamsg(u32 channel)
{
    if (channel >= FIFO_NUM_CHANNELS)
        return false;

    int oldIME = enterCriticalSection();
    fifoReadRxFifoAndProcessBuffer();
    leaveCriticalSection(oldIME);

    return fifo_data_queue[channel].head != FIFO_BUFFER_TERMINATE;
}

int fifoCheckDatamsgLength(u32 channel)
{
    if (channel >= FIFO_NUM_CHANNELS)
        return -1;

    int oldIME = enterCriticalSection();
    fifoReadRxFifoAndProcessBuffer();
    leaveCriticalSection(oldIME);

    if (!fifoCheckDatamsg(channel))
        return -1;

    int block = fifo_data_queue[channel].head;
    return POOL_EXTRA(block);
}

bool fifoCheckValue32(u32 channel)
{
    if (channel >= FIFO_NUM_CHANNELS)
        return false;

    int oldIME = enterCriticalSection();
    fifoReadRxFifoAndProcessBuffer();
    leaveCriticalSection(oldIME);

    return fifo_value32_queue[channel].head != FIFO_BUFFER_TERMINATE;
}

// Interrupt handlers
// ------------------

// This interrupt is called whenever the RX FIFO hardware registers have words
// ready to be read.
static void fifoInternalRecvInterrupt(void)
{
    fifoReadRxFifoAndProcessBuffer();
}

// This interrupt handler is called when the TX FIFO hardware registers become
// empty. This means that the user has enqueued too many words to be sent and
// they didn't fill in the hardware TX registers the first time.
static void fifoInternalSendInterrupt(void)
{
    fifoFillTxFifoFromBuffer();
}

// Initialization code
// -------------------

bool fifoInit(void)
{
    // Clear all the words that were being sent to the other CPU
    REG_IPC_FIFO_CR = IPC_FIFO_SEND_CLEAR;

    // Configure individual queues for each FIFO channel. Mark them as empty.
    for (int i = 0; i < FIFO_NUM_CHANNELS; i++)
    {
        fifo_address_queue[i].head = FIFO_BUFFER_TERMINATE;
        fifo_address_queue[i].tail = FIFO_BUFFER_TERMINATE;

        fifo_data_queue[i].head = FIFO_BUFFER_TERMINATE;
        fifo_data_queue[i].tail = FIFO_BUFFER_TERMINATE;

        fifo_value32_queue[i].head = FIFO_BUFFER_TERMINATE;
        fifo_value32_queue[i].tail = FIFO_BUFFER_TERMINATE;

        fifo_address_data[i] = NULL;
        fifo_value32_data[i] = NULL;
        fifo_datamsg_data[i] = NULL;

        fifo_address_func[i] = NULL;
        fifo_value32_func[i] = NULL;
        fifo_datamsg_func[i] = NULL;
    }

    // Configure all the global buffer as empty. All entries are unused. Also,
    // all of them point to the next entry except for the last one, which
    // terminates the queue.
    for (int i = 0; i < FIFO_BUFFER_ENTRIES - 1; i++)
    {
        POOL_DATA(i) = 0;
        POOL_NEXT(i) = i + 1;
        POOL_EXTRA(i) = 0;
    }

    POOL_DATA(FIFO_BUFFER_ENTRIES - 1) = 0;
    POOL_NEXT(FIFO_BUFFER_ENTRIES - 1) = FIFO_BUFFER_TERMINATE;
    POOL_EXTRA(FIFO_BUFFER_ENTRIES - 1) = 0;

    // Functions fifo_buffer_alloc_block() and fifo_buffer_free_block() can't
    // setup fifo_free_queue.head and fifo_free_queue.tail once the last entry
    // in the queue disappears. It's important to pretend that the buffer has
    // one fewer entry than it really has so that the queue never disappears,
    // simplifying the allocation/free code.
    global_pool_free_words = FIFO_BUFFER_ENTRIES - 1;

    // Setup the queue of free entries to span the whole queue
    fifo_free_queue.head = 0;
    fifo_free_queue.tail = FIFO_BUFFER_ENTRIES - 1;

    // Set the TX and RX queues as empty
    fifo_tx_queue.head = FIFO_BUFFER_TERMINATE;
    fifo_tx_queue.tail = FIFO_BUFFER_TERMINATE;

    fifo_rx_queue.head = FIFO_BUFFER_TERMINATE;
    fifo_rx_queue.tail = FIFO_BUFFER_TERMINATE;

    // Setup interrupt handlers
    irqSet(IRQ_SEND_FIFO, fifoInternalSendInterrupt);
    irqSet(IRQ_RECV_FIFO, fifoInternalRecvInterrupt);
    REG_IPC_FIFO_CR = IPC_FIFO_ENABLE | IPC_FIFO_RECV_NOT_EMPTY_IRQ;
    irqEnable(IRQ_RECV_FIFO | IRQ_SEND_FIFO);

    return true;
}

#ifdef ARM9

// Helpers to prevent multiple threads from using the same FIFO channel
// --------------------------------------------------------------------

static comutex_t fifo_mutex[FIFO_NUM_CHANNELS];

void fifoMutexAcquire(u32 channel)
{
    if (channel >= FIFO_NUM_CHANNELS)
        return;

    comutex_acquire(&fifo_mutex[channel]);
}

bool fifoMutexTryAcquire(u32 channel)
{
    if (channel >= FIFO_NUM_CHANNELS)
        return false;

    return comutex_try_acquire(&fifo_mutex[channel]);
}

void fifoMutexRelease(u32 channel)
{
    if (channel >= FIFO_NUM_CHANNELS)
        return;

    comutex_release(&fifo_mutex[channel]);
}

#endif // ARM9
