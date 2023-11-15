// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2005 Jason Rogers (dovoto)
// Copyright (C) 2005 Dave Murphy (WinterMute)

// Interrupt registers and vector pointers

/// @file nds/interrupts.h
///
/// @brief nds interrupt support.

#ifndef LIBNDS_NDS_INTERRUPTS_H__
#define LIBNDS_NDS_INTERRUPTS_H__

#include <nds/ndstypes.h>

// Values allowed for REG_IE and REG_IF
#define IRQ_VBLANK          BIT(0)  ///< Vertical blank interrupt mask
#define IRQ_HBLANK          BIT(1)  ///< Horizontal blank interrupt mask
#define IRQ_VCOUNT          BIT(2)  ///< Vcount match interrupt mask
#define IRQ_TIMER0          BIT(3)  ///< Timer 0 interrupt mask
#define IRQ_TIMER1          BIT(4)  ///< Timer 1 interrupt mask
#define IRQ_TIMER2          BIT(5)  ///< Timer 2 interrupt mask
#define IRQ_TIMER3          BIT(6)  ///< Timer 3 interrupt mask
#ifdef ARM7
#define IRQ_NETWORK         BIT(7)  ///< Serial/RTC interrupt mask (ARM7) (deprecated name)
#define IRQ_RTC             BIT(7)  ///< Serial/RTC interrupt mask (ARM7)
#endif
#define IRQ_DMA0            BIT(8)  ///< DMA 0 interrupt mask
#define IRQ_DMA1            BIT(9)  ///< DMA 1 interrupt mask
#define IRQ_DMA2            BIT(10) ///< DMA 2 interrupt mask
#define IRQ_DMA3            BIT(11) ///< DMA 3 interrupt mask
#define IRQ_KEYS            BIT(12) ///< Keypad interrupt mask
#define IRQ_CART            BIT(13) ///< GBA cartridge interrupt mask
#define IRQ_IPC_SYNC        BIT(16) ///< IPC sync interrupt mask
#define IRQ_FIFO_EMPTY      BIT(17) ///< Send FIFO empty interrupt mask
#define IRQ_FIFO_NOT_EMPTY  BIT(18) ///< Receive FIFO not empty interrupt mask
#define IRQ_CARD            BIT(19) ///< interrupt mask DS Card Slot
#define IRQ_CARD_LINE       BIT(20) ///< interrupt mask
#ifdef ARM9
#define IRQ_GEOMETRY_FIFO   BIT(21) ///< Geometry FIFO interrupt mask (ARM9)
#define IRQ_DSP             BIT(24) ///< DSP interrupt mask (DSi ARM9)
#define IRQ_CAMERA          BIT(25) ///< Camera interrupt mask (DSi ARM9)
#endif
#ifdef ARM7
#define IRQ_LID             BIT(22) ///< Hinge open interrupt mask
#define IRQ_SPI             BIT(23) ///< SPI interrupt mask
#define IRQ_WIFI            BIT(24) ///< WIFI interrupt mask (ARM7)
#endif
#define IRQ_NDMA0           BIT(28) ///< NDMA 0 interrupt mask (DSi)
#define IRQ_NDMA1           BIT(29) ///< NDMA 1 interrupt mask (DSi)
#define IRQ_NDMA2           BIT(30) ///< NDMA 2 interrupt mask (DSi)
#define IRQ_NDMA3           BIT(31) ///< NDMA 3 interrupt mask (DSi)
#define IRQ_ALL             (~0)    ///< Mask for all interrupts

typedef uint32_t IRQ_MASKS;
typedef uint32_t IRQ_MASK;

#ifdef ARM7
// Values allowed for REG_AUXIE and REG_AUXIF

#define IRQ_HEADPHONE       BIT(5)  ///< Headphone interrupt mask (DSi ARM7)
#define IRQ_I2C             BIT(6)  ///< I2C interrupt mask (DSi ARM7)
#define IRQ_SDMMC           BIT(8)  ///< SD/MMC controller interrupt mask (DSi ARM7)
#define IRQ_SD_DATA         BIT(9)  ///< SD/MMC data interrupt mask (DSi ARM7)
#define IRQ_SDIO            BIT(10) ///< SDIO controller interrupt mask (DSi ARM7)
#define IRQ_SDIO_DATA       BIT(11) ///< SDIO data interrupt mask (DSi ARM7)
#define IRQ_AES             BIT(12) ///< AES interrupt mask (DSi ARM7)
// TODO: bit 13 (second DSi ARM7 I2C interrupt)
#define IRQ_MICEXT          BIT(14) ///< microphone interrupt mask (DSi ARM7)

typedef uint32_t IRQ_MASKSAUX;
#endif

/// Returns the mask for a given timer.
///
/// @param n Timer index.
/// @return Bitmask.
#define IRQ_TIMER(n)    (1 << ((n) + 3))

#define IRQ_DMA(n)      (1 << ((n) + 8))
#define IRQ_NDMA(n)     (1 << ((n) + 28))

/// Maximum number of interrupts.
#define MAX_INTERRUPTS      32
#ifdef ARM7
#define MAX_INTERRUPTS_AUX  15
#endif

/// Interrupt Enable Register.
///
/// This is the activation mask for the internal interrupts. Unless the
/// corresponding bit is set, the IRQ will be masked out.
#define REG_IE      (*(vuint32 *)0x04000210)
#ifdef ARM7
#define REG_AUXIE   (*(vuint32 *)0x04000218)
#endif

/// Interrupt Flag Register.
///
/// Since there is only one hardware interrupt vector, the IF register contains
/// flags to indicate when a particular of interrupt has occured. To acknowledge
/// processing interrupts, set IF to the value of the interrupt handled.
#define REG_IF      (*(vuint32 *)0x04000214)
#ifdef ARM7
#define REG_AUXIF   (*(vuint32 *)0x0400021C)
#endif

/// Interrupt Master Enable Register.
///
/// When bit 0 is clear, all interrupts are masked. When it is 1, interrupts
/// will occur if not masked out in REG_IE.
#define REG_IME     (*(vuint32 *)0x04000208)

/// Values allowed for REG_IME
enum IME_VALUE {
    IME_DISABLE = 0, ///< Disable all interrupts.
    IME_ENABLE = 1,  ///< Enable all interrupts not masked out in REG_IE
};

#ifdef __cplusplus
extern "C" {
#endif

extern VoidFn  __irq_vector[];
extern vuint32 __irq_flags[];
extern vuint32 __irq_flagsaux[];

#define INTR_WAIT_FLAGS     *(__irq_flags)
#define INTR_WAIT_FLAGSAUX  *(__irq_flagsaux)
#define IRQ_HANDLER         *(__irq_vector)

struct IntTable{IntFn handler; u32 mask;};

/// Initialise the libnds interrupt system.
///
/// This function is called internally (prior to main()) to set up IRQs on the
/// ARM9. It must be called on the ARM7 prior to installing IRQ handlers.
void irqInit(void);

/// Add a handler for the given interrupt mask.
///
/// Specify the handler to use for the given interrupt. This only works with the
/// default interrupt handler, do not mix the use of this routine with a
/// user-installed IRQ handler.
///
/// @param irq Mask associated with the interrupt.
/// @param handler Address of the function to use as an interrupt service
///                routine
/// @note When any handler specifies using IRQ_VBLANK or IRQ_HBLANK, DISP_SR is
///       automatically updated to include the corresponding DISP_VBLANK_IRQ or
///       DISP_HBLANK_IRQ.
/// @warning Only one IRQ_MASK can be specified with this function.
void irqSet(u32 irq, VoidFn handler);
#ifdef ARM7
void irqSetAUX(u32 irq, VoidFn handler);
#endif

/// Remove the handler associated with the interrupt mask IRQ.
///
/// @param irq Mask associated with the interrupt.
void irqClear(u32 irq);
#ifdef ARM7
void irqClearAUX(u32 irq);
#endif

/// Install a user interrupt dispatcher.
///
/// This function installs the main interrupt function, all interrupts are
/// serviced through this routine. For most purposes, the libnds interrupt
/// dispacther should be used in preference to user code unless you know
/// *exactly* what you're doing.
///
/// @param handler Address of the function to use as an interrupt dispatcher.
/// @note The function must be ARM code.
void irqInitHandler(VoidFn handler);

/// Allow the given interrupt to occur.
///
/// @param irq The set of interrupt masks to enable.
/// @note Specify multiple interrupts to enable by ORing several IRQ_MASKS.
void irqEnable(u32 irq);
#ifdef ARM7
void irqEnableAUX(u32 irq);
#endif

/// Prevent the given interrupt from occuring.
///
/// @param irq The set of interrupt masks to disable.
/// @note Specify multiple interrupts to disable by ORing several IRQ_MASKS.
void irqDisable(u32 irq);
#ifdef ARM7
void irqDisableAUX(u32 irq);
#endif

/// Wait for interrupt(s) to occur.
///
/// @param waitForSet 0: Return if the interrupt has already occured; 1: Wait
///                   until the interrupt has been set since the call
/// @param flags Interrupt mask to wait for.
void swiIntrWait(u32 waitForSet, uint32_t flags);

/// Waits for a vertical blank interrupt
///
/// @note Identical to calling swiIntrWait(1, 1)
void swiWaitForVBlank(void);

/// Set callback for DSi Powerbutton press
///
/// @param CB Function to call when power button pressed
/// @return The previously set callback
VoidFn setPowerButtonCB(VoidFn CB);

static inline int enterCriticalSection(void)
{
    int oldIME = REG_IME;
    REG_IME = 0;
    return oldIME;
}

static inline void leaveCriticalSection(int oldIME)
{
    REG_IME = oldIME;
}

#ifdef __cplusplus
}
#endif

#endif // LIBNDS_NDS_INTERRUPTS_H__
