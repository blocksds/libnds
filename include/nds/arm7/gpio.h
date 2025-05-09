// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2023 Adrian "asie" Siekierka

#ifndef LIBNDS_NDS_ARM7_GPIO_H__
#define LIBNDS_NDS_ARM7_GPIO_H__

#ifdef __cplusplus
extern "C" {
#endif

#ifndef ARM7
#error GPIO header is for ARM7 only
#endif

/// @file nds/arm7/gpio.h
///
/// @brief DSi GPIO ARM7 definitions and helpers.
///
/// Note that these are accessible in both DS and DSi mode.

#include <nds/ndstypes.h>

#define REG_GPIO_DATA            (*(vu8 *) 0x4004C00)
#define REG_GPIO_DIR             (*(vu8 *) 0x4004C01)
#define REG_GPIO_IRQ_EDGE        (*(vu8 *) 0x4004C02)
#define REG_GPIO_IRQ_ENABLE      (*(vu8 *) 0x4004C03)
#define REG_GPIO_WIFI            (*(vu16 *)0x4004C04)

#define GPIO_18_0                BIT(0)
#define GPIO_18_1                BIT(1)
#define GPIO_18_2                BIT(2)
#define GPIO_33_0                BIT(4)
#define GPIO_33_1                BIT(5)
#define GPIO_33_2                BIT(6)
#define GPIO_33_3                BIT(7)

#define GPIO_HEADPHONE_CONNECT   GPIO_33_1
#define GPIO_POWER_BUTTON_IRQ    GPIO_33_2
#define GPIO_SOUND_ENABLE_OUTPUT GPIO_33_3

#define GPIO_WIFI_MODE_TWL       (0)    ///< TWL/DSi WiFi mode
#define GPIO_WIFI_MODE_NTR       BIT(8) ///< NTR/NDS WiFi mode
#define GPIO_WIFI_MODE_MASK      BIT(8)

/// Set the GPIO Wi-Fi chipset mode.
///
/// Using the NDS Wi-Fi chip requires GPIO_WIFI_MODE_NTR on DSi consoles.
/// On DSi, DWM-W024 boards require GPIO_WIFI_MODE_TWL to correctly operate.
///
/// @param mode
///     Requested mode. @see GPIO_WIFI_MODE_TWL or @see GPIO_WIFI_MODE_NTR
void gpioSetWifiMode(u16 mode);

#ifdef __cplusplus
}
#endif

#endif
