// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2005 Jason Rogers (dovoto)
// Copyright (C) 2005 Dave Murphy (WinterMute)

// Power control, keys, and HV clock registers

/// @file nds/system.h
///
/// @brief NDS hardware definitions.
///
/// These definitions are usually only touched during the initialization of the
/// program.

#ifndef LIBNDS_NDS_SYSTEM_H__
#define LIBNDS_NDS_SYSTEM_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

#include <nds/fifocommon.h>
#include <nds/ndstypes.h>

/// Screen height in pixels.
#define SCREEN_HEIGHT 192

/// Screen width in pixels.
#define SCREEN_WIDTH  256

/// LCD status register.
#define REG_DISPSTAT (*(vu16*)0x04000004)

/// LCD Status register defines
typedef enum
{
    DISP_IN_VBLANK    = BIT(0), ///< The display currently in a vertical blank.
    DISP_IN_HBLANK    = BIT(1), ///< The display currently in a horizontal blank.
    DISP_YTRIGGERED   = BIT(2), ///< Current scanline and DISP_Y match.
    DISP_VBLANK_IRQ   = BIT(3), ///< Interrupt on vertical blank.
    DISP_HBLANK_IRQ   = BIT(4), ///< Interrupt on horizontal blank.
    DISP_YTRIGGER_IRQ = BIT(5)  ///< Interrupt when current scanline and DISP_Y match.
} DISP_BITS;

/// Current display scanline.
#define REG_VCOUNT (*(vu16*)0x4000006)

/// Halt control register.
///
/// Writing 0x40 to REG_HALTCNT activates GBA mode. REG_HALTCNT can only be accessed via
/// the BIOS.
#define REG_HALTCNT (*(vu16*)0x04000300)

/// Power control register.
///
/// This register controls what hardware should be turned ON or OFF.
#define REG_POWERCNT *(vu16 *)0x4000304

/// Sets the LCD refresh scanline Y trigger
///
/// @param Yvalue
///     The value for the Y trigger.
static inline void SetYtrigger(int Yvalue)
{
    REG_DISPSTAT = (REG_DISPSTAT & 0x007F) | (Yvalue << 8) | ((Yvalue & 0x100) >> 1);
}

/// Power Management control bits for powerOn() and powerOff().
typedef enum
{
    PM_SOUND_AMP        = BIT(0), ///< Power the sound hardware (needed for GBA mode too)
    PM_SOUND_MUTE       = BIT(1), ///< Mute the main speakers, headphone output will still work
    PM_BACKLIGHT_BOTTOM = BIT(2), ///< Enable the bottom backlight if set
    PM_BACKLIGHT_TOP    = BIT(3), ///< Enable the top backlight if set
    PM_SYSTEM_PWR       = BIT(6), ///< Turn the NDS power OFF if set

    PM_ARM9_DIRECT  = BIT(16), ///< Internal: Write to REG_POWERCNT directly instead of sending a FIFO message

    POWER_LCD       = PM_ARM9_DIRECT | BIT(0),  ///< Controls the power for both LCD screens
    POWER_2D_A      = PM_ARM9_DIRECT | BIT(1),  ///< Controls the power for the main 2D core
    POWER_MATRIX    = PM_ARM9_DIRECT | BIT(2),  ///< Controls the power for the 3D matrix
    POWER_3D_CORE   = PM_ARM9_DIRECT | BIT(3),  ///< Controls the power for the main 3D core
    POWER_2D_B      = PM_ARM9_DIRECT | BIT(9),  ///< Controls the power for the sub 2D core
    POWER_SWAP_LCDS = PM_ARM9_DIRECT | BIT(15), ///< Controls which screen should use the main core
    POWER_ALL_2D    = PM_ARM9_DIRECT | POWER_LCD | POWER_2D_A | POWER_2D_B, ///< Power 2D hardware
    POWER_ALL       = PM_ARM9_DIRECT | POWER_ALL_2D | POWER_3D_CORE | POWER_MATRIX ///< Power everything
} PM_Bits;

/// Causes the NDS to go to sleep.
///
/// The NDS will be reawakened when the lid is opened.
///
/// @note
///     By default, this is automatically called when closing the lid.
void systemSleep(void);

/// Possible states of the DS power LED.
typedef enum {
    LED_ALWAYS_ON  = 0, ///< Always ON.
    LED_BLINK_SLOW = 1, ///< Blinking slowly
    LED_BLINK_FAST = 3, ///< Blinking fast
} PM_LedStates;

/// Set the power LED blink mode.
///
/// @note This only works on DS consoles, not DSi consoles (even in DS mode).
///
/// @param state
///     New LED state.
void ledBlink(PM_LedStates state);

/// Checks whether the application is running in DSi mode.
///
/// @return
///     Returns true if the application is running in DSi mode.
static inline bool isDSiMode(void)
{
    extern bool __dsimode;
    return __dsimode;
}

/// Checks whether the application is running in a debugger or retail console.
///
/// It works on DS and DSi consoles.
///
/// @note
///     swiIsDebugger() only works with the cache disabled, this function
///     doesn't have any restrictions.
///
/// @return
///     Returns true if running on a debugger unit, false on retail units.
static inline bool isHwDebugger(void)
{
    extern bool __debugger_unit;
    return __debugger_unit;
}

/// Write bytes at a specified address to firmware flash.
///
/// On the ARM9, if the source buffer isn't in main RAM, the function will
/// allocate a temporary buffer with malloc() and free it after using it. If
/// there isn't enough memory, the function will fail.
///
/// @param address
///     Address in the firmware to write to.
/// @param buffer
///     Pointer to the buffer to write.
/// @param length
///     Size of the buffer to write.
///
/// @return
///     0 on success, else error.
int writeFirmware(u32 address, void *buffer, u32 length);

/// Read bytes at a specified address from firmware flash.
///
/// On the ARM9, if the destination buffer isn't in main RAM, the function will
/// allocate a temporary buffer with malloc() and free it after using it. If
/// there isn't enough memory, the function will fail.
///
/// @param address
///     Address in the firmware to read from.
/// @param buffer
///     Pointer to the buffer to use as destination.
/// @param length
///     Size of the buffer to read.
///
/// @return
///     0 on success, else error.
int readFirmware(u32 address, void *buffer, u32 length);

/// Turn the screen off. See systemSetBacklightLevel().
#define PM_BACKLIGHT_OFF 0
/// Set minimum brightness. See systemSetBacklightLevel().
#define PM_BACKLIGHT_MIN 1
/// Turn maximum brighness. See systemSetBacklightLevel().
#define PM_BACKLIGHT_MAX 5

/// Sets the brighness level of the screens.
///
/// This function behaves differently depending on the model of your console:
/// Some brightness levels don't work on all consoles, and this function does
/// the next best thing.
///
/// Level 0 turns the backlight off. Levels 1-5 provide different levels of
/// brightness depending on the console model. Level 5 is the maximum level of
/// brightness.
///
/// - DSi: 5 levels of brightness (1 to 5).
/// - DS Lite: 4 levels of brightness (2 to 5). Level 1 is internally set to
///   level 2.
/// - DS: The screen can be turned off or on. Levels 1 to 5 are internally set
///   to level 5 (full brightness). Some models of the DS support the same
///   levels of brightness of the DS Lite. In them, the function behaves the
///   same way as on DS Lite.
///
/// You can also use the defines PM_BACKLIGHT_OFF (0), PM_BACKLIGHT_MIN (1) and
/// PM_BACKLIGHT_MAX (5).
///
/// @note
///     DS and DS Lite consoles support turning on and off individual screens,
///     but systemSetBacklightLevel() doesn't support controlling the two
///     screens independently.
///
/// @note
///     On DSi the brightness setting is persistent and it will be the setting
///     used the next time the console is turned on.
///
/// @param level
///     Brightness level. It goes from 0 (backlight off) to 5 (max brightness).
///
/// @return
///     The new real brightness setting.
u32 systemSetBacklightLevel(u32 level);

/// Gets the DS battery level
///
/// This returns a value with two fields. Bits 0 to 3 are the battery level,
/// and bit 7 is set to 1 if an external power source is connected to the DS.
/// You can use the defines BATTERY_LEVEL_MASK and BATTERY_CHARGER_CONNECTED to
/// make your code more readable:
///
/// ```
/// u32 value = getBatteryLevel();
/// unsigned int battery_level = value & BATTERY_LEVEL_MASK;
/// bool charger_connected = value & BATTERY_CHARGER_CONNECTED;
/// ```
///
/// On DSi the battery level is the one reported by the hardware. On DS, the
/// battery level is only "high" or "low", and libnds returns 15 or 3
/// respectively as the equivalent DSi battery level. You can use defines
/// BATTERY_LEVEL_DS_HIGH and BATTERY_LEVEL_DS_LOW instead.
///
/// @note
///     When called from the ARM9, this function requests the battery
///     information to the ARM7 with a FIFO message. When called from the ARM7,
///     it is read directly.
///
/// @return
///     Battery level and external power source status.
u32 getBatteryLevel(void);

/// This is set if the charger is connected. See getBatteryLevel().
#define BATTERY_CHARGER_CONNECTED   BIT(7)

/// Mask to get the actual battery level from getBatteryLevel().
#define BATTERY_LEVEL_MASK          0xF

/// Value that corresponds to high battery level on a DS (green LED).
#define BATTERY_LEVEL_DS_HIGH       0xF
/// Value that corresponds to low battery level on a DS (red LED).
#define BATTERY_LEVEL_DS_LOW        0x3

// Helper functions for heap size

/// Returns current start of heap space.
///
/// @return
///     Returns a pointer to the start of the heap.
u8 *getHeapStart(void);

/// Returns current end of heap space.
///
/// @return
///     Returns a pointer to the current end of the heap.
u8 *getHeapEnd(void);

/// Returns current heap limit.
///
/// @return
///     Returns a pointer to the limit of the heap. It won't grow past this
///     address.
u8 *getHeapLimit(void);

/// Reduces the size of the heap from the end.
///
/// This can be useful if you need a lot of stack and you're letting the stack
/// grow so much it leaves DTCM and reaches main RAM. All of main RAM is
/// available for sbrk() and functions that use it, like malloc(). If you want
/// to let the stack grow safely you need to remove the end of main RAM from the
/// heap pool by calling reduceHeapSize().
///
/// @warning
///     You need to call this function before the heap has grown. You can check
///     getHeapEnd() to see what's the maximum memory that you can take from the
///     heap. If you free all the allocated chunks after a specific address,
///     sbrk() is called to reduce the size of the heap, which will let you call
///     reduceHeapSize() to reduce the heap limit down to that address.
///
/// @param size_to_save
///     The size that the heap needs to shrink. It must be a multiple of 4.
///
/// @return
///     Returns 0 on success, or a negative value on error.
int reduceHeapSize(size_t size_to_save);

// ARM9 section
// ------------

#ifdef ARM9

/// Turns on specified hardware.
///
/// It may be called from the ARM7 or ARM9 (ARM9 power bits will be ignored by
/// the ARM7, ARM7 power bits will be passed to the ARM7 from the ARM9).
///
/// @param bits
///     What to power ON (PM_Bits).
void powerOn(uint32_t bits);

/// Turns off specified hardware.
///
/// It may be called from the ARM7 or ARM9 (ARM9 power bits will be ignored by
/// the ARM7, ARM7 power bits will be passed to the ARM7 from the ARM9).
///
/// @param bits
///     What to power OFF (PM_Bits).
void powerOff(uint32_t bits);

/// Enables sleep mode from ARM9.
void enableSleep(void);

/// Disables sleep mode from ARM9.
void disableSleep(void);

// Internal FIFO handlers
void systemMsgHandler(int bytes, void *user_data);
void systemValueHandler(u32 value, void *data);

/// Switches the screens.
static inline void lcdSwap(void)
{
    REG_POWERCNT ^= POWER_SWAP_LCDS;
}

/// Forces the main core to display on the top.
static inline void lcdMainOnTop(void)
{
    REG_POWERCNT |= POWER_SWAP_LCDS;
}

/// Forces the main core to display on the bottom.
static inline void lcdMainOnBottom(void)
{
    REG_POWERCNT &= ~POWER_SWAP_LCDS;
}

/// Powers down the DS
static inline void systemShutDown(void)
{
    fifoSendValue32(FIFO_PM, PM_REQ_SHUTDOWN);
}

/// This function reboots the console.
///
/// It only works on DSi. On DS it does nothing.
static inline void systemReboot(void)
{
    fifoSendValue32(FIFO_PM, PM_REQ_REBOOT);
}

/// Set the ARM9 interrupt vector base to one of two locations:
///
/// - 0xFFFF0000 (default; controlled by the BIOS)
/// - 0x00000000 (alternate; configurable by the homebrew program, initialized
///   by default with function pointers to BIOS handlers)
///
/// To initialize function pointers for the alternate vector base, set the
/// relevant values in the SystemVectors structure before calling this
/// function.
///
/// Note that it is recommended to call this function with interrupts disabled
/// (REG_IME = 0).
///
/// @param base
///     Vector base. Setting it to any non-zero value will use the default
///     vector base (0xFFFF0000); setting it to zero will use the alternate
///     vector base (0x00000000).
///
/// @see SystemVectors
void setVectorBase(int base);

/// Structure of function pointers corresponding to ARM CPU interrupts.
/// Each member contains an ARM instuction that will be executed when an
/// exeption occurs.
///
/// See GBATEK for more information on each interrupt.
///
/// @see SystemVectors
typedef struct sysVectors
{
    VoidFn reset;             ///< CPU reset.
    VoidFn undefined;         ///< Undefined instruction.
    VoidFn swi;               ///< Software interrupt.
    VoidFn prefetch_abort;    ///< Prefetch abort.
    VoidFn data_abort;        ///< Data abort.
    VoidFn address_overflow;  ///< Address exceeds 26 bits. Not used on ARM9.
    VoidFn irq;               ///< Standard interrupt.
    VoidFn fiq;               ///< Fast interrupt.
} sysVectors_t;

/// Function pointers to user-provided interrupt handlers, used in the
/// alternate interrupt vector mode in place of BIOS interrupt handlers.
///
/// @see setVectorBase
extern sysVectors_t SystemVectors;

/// Set a callback to detect if an SD card is inserted or removed from a DSi.
///
/// @note
///     The callbacks are only called after the SD card hardware has been
///     initialized (for example, after calling fatInitDefault()).
///
/// @param callback
///     The callback will be called with 1 as a value if an SD card has been
///     inserted or 0 if it has been removed.
void setSDcallback(void (*callback)(int));

/// Sets the ARM9 clock speed, only possible in DSi mode.
///
/// @param speed
///     CPU speed (false = 67.03MHz, true = 134.06MHz)
///
/// @return
///     The old CPU speed value
bool setCpuClock(bool speed);

#endif // ARM9

// ARM7 section
// ------------

#ifdef ARM7

#define REG_CONSOLEID        (*(vu64 *)0x04004D00)
#define REG_CONSOLEID_FLAG   (*(vu16 *)0x04004D08)

/// Power-controlled hardware devices accessable to the ARM7.
///
/// @warning
///     This function is only available in DSi mode, so you should guard it with isDSiMode().
///
/// @return
///     Returns the console id of the DSi console.
u64 getConsoleID(void);

/// Power-controlled hardware devices accessable to the ARM7.
///
/// @note
///     These should only be used when programming for the ARM7. Trying to boot
///     up these hardware devices via the ARM9 would lead to unexpected results.
///     ARM7 only.
typedef enum {
    POWER_SOUND = BIT(0), ///< Controls the power for the sound controller
    POWER_WIFI  = BIT(1), ///< Controls the power for the WiFi controller

    PM_WRITE_REGISTER  = (0 << 7), ///< Selects PM write operation
    PM_READ_REGISTER   = (1 << 7), ///< Selects PM read operation

    PM_CONTROL_REG     = 0,     ///< Selects the PM control register
    PM_BATTERY_REG     = 1,     ///< Selects the PM battery register
    PM_AMPLIFIER_REG   = 2,     ///< Selects the PM amplifier register
    PM_AMP_OFFSET      = PM_AMPLIFIER_REG, ///< Alias of PM_AMPLIFIER_REG
    PM_GAIN_OFFSET     = 3,     ///< Selects the PM gain register
    PM_BACKLIGHT_LEVEL = 4,     ///< Selects the DS Lite backlight register
    PM_DSI_RESET_REG   = 0x10,  ///< Selects the DSi backlight mirror and reset register

    PM_GAIN_20  = 0, ///< Sets the mic gain to 20db
    PM_GAIN_40  = 1, ///< Sets the mic gain to 40db
    PM_GAIN_80  = 2, ///< Sets the mic gain to 80db
    PM_GAIN_160 = 3, ///< Sets the mic gain to 160db

    PM_AMP_ON   = 1, ///< Turns the sound amp ON
    PM_AMP_OFF  = 0  ///< Turns the sound amp OFF
} ARM7_power;

/// PM control register bits - LED control
#define PM_LED_CONTROL_MASK (3 << 4)
#define PM_LED_CONTROL(m)   ((m) << 4)

/// Install the system FIFO handlers.
///
/// This handles power management, DSi SD card access, and firmware flash
/// access.
void installSystemFIFO(void);

// Internal. Check if sleep mode is enabled.
int sleepEnabled(void);

/// Write to a power management register.
///
/// @param reg
///     Register address.
/// @param command
///     Command to send to the register.
///
/// @return
///     Value returned from the command/value exchange.
int writePowerManagement(int reg, int command);

/// Read from a power management register.
///
/// @param reg
///     The register to read from.
///
/// @return
///     The value read from the register.
static inline int readPowerManagement(int reg)
{
    return writePowerManagement(reg | PM_READ_REGISTER, 0);
}

static inline void powerOn(uint32_t bits)
{
    REG_POWERCNT |= bits;
}

static inline void powerOff(uint32_t bits)
{
    REG_POWERCNT &= ~bits;
}

/// Read user settings/personal data from firmware flash to a shared memory
/// location.
///
/// @return
///     It returns true on success, false on error.
bool readUserSettings(void);

/// This function shuts down the console.
///
/// If it fails, it returns.
void systemShutDown(void);

/// This function reboots the console.
///
/// It only works on DSi. On DS it does nothing.
void systemReboot(void);

#endif // ARM7

/// DS-Lite firmware backlight level settings.
///
/// @warning
///     Only available on DS Lite.
typedef enum {
    BACKLIGHT_LOW,  ///< Low backlight setting.
    BACKLIGHT_MED,  ///< Medium backlight setting.
    BACKLIGHT_HIGH, ///< High backlight setting.
    BACKLIGHT_MAX   ///< Max backlight setting.
} BACKLIGHT_LEVELS;

// Common functions

/// User's DS settings.
///
/// Defines the structure the DS firmware uses for transfer of the user's
/// settings to the booted program.
///
/// @warning
///     This struct is initialized by the ARM7 when readUserSettings() is
///     called. This function is called by default at the beginning of main(),
///     but this means that the personal data isn't accessible from the ARM9
///     right at the start of main(). It may take a frame or two to be
///     actually available.
typedef struct tPERSONAL_DATA
{
    u8 RESERVED0[2]; // Version (0x05, 0x00)

    /// The user's theme color (0-15).
    ///
    /// Theme/Color values:
    /// - 0 = Gray
    /// - 1 = Brown
    /// - 2 = Red
    /// - 3 = Pink
    /// - 4 = Orange
    /// - 5 = Yellow
    /// - 6 = Yellow/Green-ish
    /// - 7 = Green
    /// - 8 = Dark Green
    /// - 9 = Green/Blue-ish
    /// - 10 = Light Blue
    /// - 11 = Blue
    /// - 12 = Dark Blue
    /// - 13 = Dark Purple
    /// - 14 = Purple
    /// - 15 = Purple/Red-ish
    u8 theme;

    u8 birthMonth;      ///< The user's birth month (1-12).
    u8 birthDay;        ///< The user's birth day (1-31).

    u8 RESERVED1[1];    // Not used (zero)

    s16 name[10];       ///< The user's name in UTF-16LE format.
    u16 nameLen;        ///< The length of the user's name in characters.

    s16 message[26];    ///< The user's message in UTF-16LE format.
    u16 messageLen;     ///< The length of the user's message in characters.

    u8 alarmHour;       ///< What hour the alarm clock is set to (0-23).
    u8 alarmMinute;     ///< What minute the alarm clock is set to (0-59).

    u8 RESERVED2[4];    // ??? 0x02FFFCD4  ??

    u16 calX1;          ///< Touchscreen calibration: first X touch
    u16 calY1;          ///< Touchscreen calibration: first Y touch
    u8 calX1px;         ///< Touchscreen calibration: first X touch pixel
    u8 calY1px;         ///< Touchscreen calibration: first X touch pixel

    u16 calX2;          ///< Touchscreen calibration: second X touch
    u16 calY2;          ///< Touchscreen calibration: second Y touch
    u8 calX2px;         ///< Touchscreen calibration: second X touch pixel
    u8 calY2px;         ///< Touchscreen calibration: second Y touch pixel

    struct {
        /// User's language.
        ///
        /// Language values:
        /// - 0 = Japanese
        /// - 1 = English
        /// - 2 = French
        /// - 3 = German
        /// - 4 = Italian
        /// - 5 = Spanish
        /// - 6 = Chinese(?)
        /// - 7 = Unknown/Reserved
        u32 language          : 3;

        /// GBA screen selection (lower screen if set, otherwise upper screen).
        u32 gbaScreen         : 1;

        /// Brightness level at power on, DS Lite. See BACKLIGHT_LEVELS.
        u32 defaultBrightness : 2;

        /// The DS should boot from the DS cart or GBA cart automatically if one is inserted.
        u32 autoMode          : 1;

        u32 RESERVED5         : 2;

        /// User Settings Lost (0 = Normal, 1 = Prompt/Settings Lost)
        u32 settingsLost      : 1;

        u32 RESERVED6         : 6;
    } PACKED;

    u8 year;            ///< Year (0 = 2000 .. 255 = 2255)
    u8 rtcClockAdjust;  ///< Real Time Clock adjustment register value.

    /// Real Time Clock offset.
    ///
    /// Whenever the time/date of the NDS is changed in the system settings menu
    /// from time A to time B, the firmware adds (epoch(B) - epoch(A)) to this
    /// field. libnds doesn't do this in rtcTimeSet() or rtcTimeAndDateSet(),
    /// so any game that relies on this firmware field to detect time/date
    /// modifications won't be able to do it.
    u32 rtcOffset;

    u32 RESERVED4; // ???
} PACKED PERSONAL_DATA;

/// Default location for the user's personal data (see PERSONAL_DATA).
#define PersonalData ((PERSONAL_DATA *)0x2FFFC80)

/// Struct containing time and day of the real time clock.
///
/// Use rtcTimeAndDate instead.
__attribute__((deprecated)) typedef struct {
    u8 year;    ///< Add 2000 to get 4 digit year
    u8 month;   ///< 1 to 12
    u8 day;     ///< 1 to (days in month)
    u8 weekday; ///< Day of week (0 = Sunday, 1 = Monday, ...,  6 = Saturday)
    u8 hours;   ///< 0 to 11 for AM, 52 to 63 for PM
    u8 minutes; ///< 0 to 59
    u8 seconds; ///< 0 to 59
} RTCtime;

/// Struct containing time and day of the real time clock.
typedef struct {
    u8 year;    ///< Add 2000 to get 4 digit year
    u8 month;   ///< 1 to 12
    u8 day;     ///< 1 to (days in month)
    u8 weekday; ///< Day of week (0 = Sunday, 1 = Monday, ...,  6 = Saturday)
    u8 hours;   ///< 0 to 11 for AM, 52 to 63 for PM
    u8 minutes; ///< 0 to 59
    u8 seconds; ///< 0 to 59
} rtcTimeAndDate;

/// Struct containing time of the real time clock.
typedef struct {
    u8 hours;   ///< 0 to 11 for AM, 52 to 63 for PM
    u8 minutes; ///< 0 to 59
    u8 seconds; ///< 0 to 59
} rtcTime;

// argv struct magic number
#define ARGV_MAGIC 0x5f617267

// Structure used to set up argc/argv on the DS
struct __argv {
    int argvMagic;      // argv magic number, set to 0x5f617267 ('_arg') if valid
    char *commandLine;  // Base address of command line, set of null terminated strings
    int length;         // Total length of command line
    int argc;           // Internal use, number of arguments
    char **argv;        // Internal use, argv pointer
    char **endARGV;     // Internal use, pointer to the end of argv in the heap
    u32 host;           // Internal use, host ip for dslink
};

#define ARGV_ADDRESS ((u32)0x02FFFE70)
#define __system_argv ((struct __argv *)ARGV_ADDRESS)

#define BOOTSIG 0x62757473746F6F62ULL // 'bootstub'

struct __bootstub {
    u64 bootsig;
    VoidFn arm9reboot;
    VoidFn arm7reboot;
    u32 bootsize;
};

// This is 0x23F4000 on NDS and 0x2FF4000 on DSi. However, on NDS 0x2FF4000 is a
// mirror of 0x23F4000.
#define __system_bootstub ((struct __bootstub *)0x02FF4000)

#ifdef ARM9
/// Returns a cached mirror of an address.
///
/// @param address
///     An address.
///
/// @return
///     A pointer to the cached mirror of that address.
void *memCached(void *address);

/// Returns an uncached mirror of an address.
///
/// @param address
///     An address.
///
/// @return
///     A pointer to the uncached mirror of that address.
void *memUncached(void *address);

/// Checks if a buffer is inside main RAM or not.
///
/// @param buffer
///     Pointer to the buffer to check.
/// @param size
///     Size of the buffer.
///
/// @return
///     It returns true if the buffer fits in main RAM, false otherwise.
bool memBufferIsInMainRam(const void *buffer, size_t size);

/// Enable data cache for the DS slot-2 memory region.
///
/// Note that this is not safe to enable if you're using Slot-2 memory for
/// purposes other than external RAM, such as rumble or other I/O.
///
/// @param write_back
///     Set as write-back. This allows writes to skip the slow Slot-2 bus, at
///     the expense of requiring a full memory flush when calling
///     peripheralSlot2DisableCache().
void peripheralSlot2EnableCache(bool write_back);

/// Disable data cache for the DS slot-2 memory region.
/// If write-back was enabled, additionally clear and flush the data cache.
///
/// Note that flushing the data cache may still be required to remove stale
/// "read" pages, such as if you write to external RAM uncached and then
/// re-enable cache.
void peripheralSlot2DisableCache(void);

void resetARM7(u32 address);
#endif

#ifdef ARM7
void resetARM9(u32 address);
#endif

// DSi SCFG registers

// SCFG_xxROM
// ==========

#define REG_SCFG_ROM            (*(vu16 *)0x4004000)

#ifdef ARM7
#define REG_SCFG_A9ROM          (*(vu8 *)0x4004000)
#define REG_SCFG_A7ROM          (*(vu8 *)0x4004001)
#endif // ARM7

// SCFG_CLK
// ========

#define REG_SCFG_CLK            (*(vu16 *)0x4004004)

#ifdef ARM9
#define SCFG_CLK_ARM9_TWL       BIT(0)
#define SCFG_CLK_DSP            BIT(1)
#define SCFG_CLK_CAMERA_IF      BIT(2)
#define SCFG_CLK_NWRAM          BIT(7) // Read only, set by ARM7
#define SCFG_CLK_CAMERA_EXT     BIT(8)
#endif // ARM9

#ifdef ARM7
#define SCFG_CLK_SDMMC          BIT(0)
#define SCFG_CLK_AES            BIT(2)
#define SCFG_CLK_NWRAM          BIT(7)
#define SCFG_CLK_TOUCH          BIT(8)
#endif // ARM7

// SCFG_RST
// ========

#ifdef ARM9
#define REG_SCFG_RST            (*(vu16 *)0x4004006)

#define SCFG_RST_DSP_APPLY      (0 << 0)
#define SCFG_RST_DSP_RELEASE    (1 << 0)
#endif // ARM9

// SCFG_JTAG
// =========

#ifdef ARM7
#define REG_SCFG_JTAG           (*(vu16 *)0x4004006)

#define SCFG_JTAG_ARM7SEL       (1 << 0)
#define SCFG_JTAG_CPU_ENABLE    (1 << 1)
#define SCFG_JTAG_DSP_ENABLE    (1 << 8)
#endif // ARM7

// SCFG_EXT
// ========

#define REG_SCFG_EXT            (*(vu32 *)0x4004008)

#ifdef ARM9
#define SCFG_EXT_DMA            BIT(0)
#define SCFG_EXT_GEOMETRY       BIT(1)
#define SCFG_EXT_RENDERER       BIT(2)
#define SCFG_EXT_2D             BIT(3)
#define SCFG_EXT_DIVIDER        BIT(4)
#define SCFG_EXT_CARD           BIT(7)
#define SCFG_EXT_INTERRUPT      BIT(8)
#define SCFG_EXT_LCD            BIT(12)
#define SCFG_EXT_VRAM           BIT(13)
#define SCFG_EXT_RAM_DEBUG      BIT(14)
#define SCFG_EXT_RAM_TWL        BIT(15)
#define SCFG_EXT_NDMA           BIT(16)
#define SCFG_EXT_CAMERA         BIT(17)
#define SCFG_EXT_DSP            BIT(18)
#define SCFG_EXT_MBK_RAM        BIT(25)
#define SCFG_EXT_SCFG_MBK_REG   BIT(31)
#endif // ARM9

#ifdef ARM7
#define SCFG_EXT_DMA            BIT(0)
#define SCFG_EXT_SOUND_DMA      BIT(1)
#define SCFG_EXT_SOUND          BIT(2)
#define SCFG_EXT_CARD           BIT(7)
#define SCFG_EXT_INTERRUPT      BIT(8)
#define SCFG_EXT_SPI            BIT(9)
#define SCFG_EXT_SOUND_DMA_EXT  BIT(10)
#define SCFG_EXT_LCD            BIT(12)
#define SCFG_EXT_VRAM           BIT(13)
#define SCFG_EXT_RAM_DEBUG      BIT(14)
#define SCFG_EXT_RAM_TWL        BIT(15)
#define SCFG_EXT_NDMA           BIT(16)
#define SCFG_EXT_AES            BIT(17)
#define SCFG_EXT_SDMMC          BIT(18)
#define SCFG_EXT_WIFI_SDIO      BIT(19)
#define SCFG_EXT_MIC            BIT(20)
#define SCFG_EXT_SNDEXCNT       BIT(21)
#define SCFG_EXT_I2C            BIT(22)
#define SCFG_EXT_GPIO           BIT(23)
#define SCFG_EXT_MBK_RAM        BIT(25)
#define SCFG_EXT_SCFG_MBK_REG   BIT(31)
#endif // ARM7

// SCFG_MC
// =======

#define REG_SCFG_MC             (*(vu16 *)0x4004010)

#define SCFG_MC_EJECTED         0x01
#define SCFG_MC_PWR_MASK        0x0C
#define SCFG_MC_PWR_OFF         0x00
#define SCFG_MC_PWR_RESET       0x04
#define SCFG_MC_PWR_ON          0x08
#define SCFG_MC_PWR_REQUEST_OFF 0x0C

// SCFG_CARD_xxx
// =============

#ifdef ARM7
#define REG_SCFG_CARD_INSERT_DELAY      (*(vu16 *)0x4004012)
#define REG_SCFG_CARD_PWROFF_DELAY      (*(vu16 *)0x4004014)

#define SCFG_CARD_INSERT_DELAY_DEFAULT  0x1988 // 100 ms
#define SCFG_CARD_PWROFF_DELAY_DEFAULT  0x264C // 150 ms
#endif

// SCFG_WL
// =======

#ifdef ARM7
#define REG_SCFG_WL             (*(vu16 *)0x4004020)

#define SCFG_WL_OFFB            (1 << 0)
#endif // ARM7

// SCFG_OP
// =======
//
#ifdef ARM7
#define REG_SCFG_OP             (*(vu16 *)0x4004024)

#define SCFG_OP_IS_DEBUG        (1 << 0)
#define SCFG_OP_UNKNOWN         (1 << 4)
#endif // ARM7

#ifdef __cplusplus
}
#endif

#endif // LIBNDS_NDS_SYSTEM_H__
