// SPDX-License-Identifier: Zlib
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

#include <nds/ndstypes.h>

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
/// Writing 0x40 to HALT_CR activates GBA mode. HALT_CR can only be accessed via
/// the BIOS.
#define HALT_CR (*(vu16*)0x04000300)

/// Power control register.
///
/// This register controls what hardware should be turned ON or OFF.
#define REG_POWERCNT *(vu16 *)0x4000304

/// Sets the LCD refresh scanline Y trigger
///
/// @param Yvalue The value for the Y trigger.
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
/// @note By default, this is automatically called when closing the lid.
void systemSleep(void);

/// Set the LED blink mode
///
/// @param bm What to power on.
void ledBlink(int bm);

/// Checks whether the application is running in DSi mode.
///
/// @return Returns true if the application is running in DSi mode.
static inline bool isDSiMode(void)
{
    extern bool __dsimode;
    return __dsimode;
}

// ARM9 section
// ------------

#ifdef ARM9

/// Turns on specified hardware.
///
/// It may be called from the ARM7 or ARM9 (ARM9 power bits will be ignored by
/// the ARM7, ARM7 power bits will be passed to the ARM7 from the ARM9).
///
/// @param bits What to power ON (PM_Bits).
void powerOn(uint32_t bits);

/// Turns off specified hardware.
///
/// It may be called from the ARM7 or ARM9 (ARM9 power bits will be ignored by
/// the ARM7, ARM7 power bits will be passed to the ARM7 from the ARM9).
///
/// @param bits What to power OFF (PM_Bits).
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
    powerOn(PM_SYSTEM_PWR);
}

void readFirmware(u32 address, void *buffer, u32 length);
int writeFirmware(u32 address, void *buffer, u32 length);

/// Gets the DS battery level
///
/// This returns a value with two fields. Bits 0 to 3 are the battery level,
/// and bit 7 is set to 1 if an external power source is connected to the DS.
///
/// On DSi the battery level is the one reported by the hardware. On DS, the
/// battery level is only "high" or "low", and libnds returns 15 or 3
/// respectively as the equivalent DSi battery level.
///
/// @return Battery level and external power source status.
u32 getBatteryLevel(void);

/// Set the arm9 vector base
///
/// @param highVector High vector.
/// @note ARM9 only
void setVectorBase(int highVector);

/// A struct with all the CPU exeption vectors.
///
/// Each member contains an ARM instuction that will be executed when an
/// exeption occurs.
///
/// See gbatek for more information.
typedef struct sysVectors_t {
    VoidFn reset;           ///< CPU reset.
    VoidFn undefined;       ///< Undefined instruction.
    VoidFn swi;             ///< Software interrupt.
    VoidFn prefetch_abort;  ///< Prefetch abort.
    VoidFn data_abort;      ///< Data abort.
    VoidFn fiq;             ///< Fast interrupt.
} sysVectors;

extern sysVectors SystemVectors;

void setSDcallback(void (*callback)(int));

/// Sets the ARM9 clock speed, only possible in DSi mode.
///
/// @param speed CPU speed (false = 67.03MHz, true = 134.06MHz)
/// @return The old CPU speed value
bool setCpuClock(bool speed);

// Helper functions for heap size

/// Returns current start of heap space.
///
/// @return Returns a pointer to the start of the heap.
u8 *getHeapStart(void);

/// Returns current end of heap space.
///
/// @return Returns a pointer to the end of the heap.
u8 *getHeapEnd(void);

/// Returns current heap limit.
///
/// @return Returns a pointer to the limit of the heap. It won't grow past this
/// address.
u8 *getHeapLimit(void);

#endif // ARM9

// ARM7 section
// ------------

#ifdef ARM7

#define REG_CONSOLEID   (*(vu64 *)0x04004D00)

/// Power-controlled hardware devices accessable to the ARM7.
///
/// @note These should only be used when programming for the ARM7. Trying to
/// boot up these hardware devices via the ARM9 would lead to unexpected
/// results. ARM7 only.
typedef enum {
    POWER_SOUND = BIT(0),          ///< Controls the power for the sound controller

    PM_CONTROL_REG     = 0,        ///< Selects the PM control register
    PM_BATTERY_REG     = 1,        ///< Selects the PM battery register
    PM_AMPLIFIER_REG   = 2,        ///< Selects the PM amplifier register
    PM_READ_REGISTER   = (1 << 7), ///< Selects the PM read register
    PM_AMP_OFFSET      = 2,        ///< Selects the PM amp register
    PM_GAIN_OFFSET     = 3,        ///< Selects the PM gain register
    PM_BACKLIGHT_LEVEL = 4,        ///< Selects the DS Lite backlight register
    PM_GAIN_20         = 0,        ///< Sets the mic gain to 20db
    PM_GAIN_40         = 1,        ///< Sets the mic gain to 40db
    PM_GAIN_80         = 2,        ///< Sets the mic gain to 80db
    PM_GAIN_160        = 3,        ///< Sets the mic gain to 160db
    PM_AMP_ON          = 1,        ///< Turns the sound amp ON
    PM_AMP_OFF         = 0         ///< Turns the sound amp OFF
} ARM7_power;

/// PM control register bits - LED control
#define PM_LED_CONTROL_MASK (3 << 4)
#define PM_LED_CONTROL(m)   ((m) << 4)

// Install the FIFO power handler.
void installSystemFIFO(void);

// Cause the DS to enter low power mode.
void systemSleep(void);

// Internal. Check if sleep mode is enabled.
int sleepEnabled(void);

// Warning: These functions use the SPI chain, and are thus 'critical' sections,
// make sure to disable interrupts during the call if you've got a VBlank IRQ
// polling the touch screen, etc...

// Read/write a power management register
int writePowerManagement(int reg, int command);

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

void readUserSettings(void);
void systemShutDown(void);

#endif // ARM7

/// Backlight level settings.
///
/// @note Only available on DS Lite.
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
typedef struct tPERSONAL_DATA
{
    u8 RESERVED0[2];    // ??? (0x05 0x00). (version according to gbatek)

    u8 theme;           ///< The user's theme color (0-15).
    u8 birthMonth;      ///< The user's birth month (1-12).
    u8 birthDay;        ///< The user's birth day (1-31).

    u8 RESERVED1[1];    // ???

    s16 name[10];       ///< The user's name in UTF-16 format.
    u16 nameLen;        ///< The length of the user's name in characters.

    s16 message[26];    ///< The user's message.
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
        u32 language          : 3; ///< User's language.
        u32 gbaScreen         : 1; ///< GBA screen selection (lower screen if set, otherwise upper screen).
        u32 defaultBrightness : 2; ///< Brightness level at power on, dslite.
        u32 autoMode          : 1; ///< The DS should boot from the DS cart or GBA cart automatically if one is inserted.
        u32 RESERVED5         : 2; // ???
        u32 settingsLost      : 1; ///< User Settings Lost (0 = Normal, 1 = Prompt/Settings Lost)
        u32 RESERVED6         : 6; // ???
    } PACKED;

    u8 year;            ///< Year (0 = 2000 .. 255 = 2255)
    u8 rtcClockAdjust;  ///< Real Time Clock adjustment register value.
    u32 rtcOffset;      ///< Real Time Clock offset.
    u32 RESERVED4;      // ???
} PACKED PERSONAL_DATA;

/// Default location for the user's personal data (see PERSONAL_DATA).
#define PersonalData ((PERSONAL_DATA *)0x2FFFC80)

/// Struct containing time and day of the real time clock.
///
/// Use rtcTimeAndDate instead.
__attribute__((deprecated)) typedef struct {
    u8 year;    ///< add 2000 to get 4 digit year
    u8 month;   ///< 1 to 12
    u8 day;     ///< 1 to (days in month)
    u8 weekday; ///< day of week
    u8 hours;   ///< 0 to 11 for AM, 52 to 63 for PM
    u8 minutes; ///< 0 to 59
    u8 seconds; ///< 0 to 59
} RTCtime;

/// Struct containing time and day of the real time clock.
typedef struct {
    u8 year;    ///< add 2000 to get 4 digit year
    u8 month;   ///< 1 to 12
    u8 day;     ///< 1 to (days in month)
    u8 weekday; ///< day of week
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

#define __system_argv ((struct __argv *)0x02FFFE70)

#define BOOTSIG 0x62757473746F6F62ULL // 'bootstub'

struct __bootstub {
    u64 bootsig;
    VoidFn arm9reboot;
    VoidFn arm7reboot;
    u32 bootsize;
};

#ifdef ARM9
/// Returns a cached mirror of an address.
///
/// @param address an address.
/// @return a pointer to the cached mirror of that address.
void *memCached(void *address);

/// Returns an uncached mirror of an address.
///
/// @param address an address.
/// @return a pointer to the uncached mirror of that address.
void *memUncached(void *address);

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

#define REG_SCFG_RST            (*(vu16 *)0x4004006)

#ifdef ARM9
#define SCFG_RST_DSP_APPLY      (0 << 0)
#define SCFG_RST_DSP_RELEASE    (1 << 0)
#endif // ARM9

// SCFG_MC
// =======

#define REG_SCFG_MC             (*(vu16 *)0x4004010)

#define SCFG_MC_EJECTED         0x01
#define SCFG_MC_PWR_MASK        0x0C
#define SCFG_MC_PWR_OFF         0x00
#define SCFG_MC_PWR_RESET       0x04
#define SCFG_MC_PWR_ON          0x08
#define SCFG_MC_PWR_REQUEST_OFF 0x0C

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

#endif // LIBNDS_NDS_SYSTEM_H__
