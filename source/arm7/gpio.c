// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2023 Adrian "asie" Siekierka

#include <nds/arm7/gpio.h>

void gpioSetWifiMode(u16 mode)
{
    if ((REG_GPIO_WIFI & GPIO_WIFI_MODE_MASK) != mode)
    {
        REG_GPIO_WIFI = (REG_GPIO_WIFI & ~GPIO_WIFI_MODE_MASK) | mode;
    }
}
