// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2005 Michael Noland (joat)
// Copyright (C) 2005 Jason Rogers (dovoto)
// Copyright (C) 2005 Dave Murphy (WinterMute)

// Microphone control for the ARM7

#ifndef LIBNDS_NDS_ARM7_TOUCH_H__
#define LIBNDS_NDS_ARM7_TOUCH_H__

#ifndef ARM7
#error Touch screen is only available on the ARM7
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include <nds/arm7/serial.h>
#include <nds/touch.h>

#define SCREEN_WIDTH            256
#define SCREEN_HEIGHT           192

#define TSC_MEASURE_TEMP1       0x84
#define TSC_MEASURE_Y           0x90
#define TSC_MEASURE_BATTERY     0xA4
#define TSC_MEASURE_Z1          0xB4
#define TSC_MEASURE_Z2          0xC4
#define TSC_MEASURE_X           0xD0
#define TSC_MEASURE_AUX         0xE4
#define TSC_MEASURE_TEMP2       0xF4

void touchInit(void);
void touchReadXY(touchPosition *touchPos);
uint16_t touchRead(uint32_t command);
uint32_t touchReadTemperature(int *t1, int *t2);
bool touchPenDown(void);

#ifdef __cplusplus
}
#endif

#endif // LIBNDS_NDS_ARM7_TOUCH_H__
