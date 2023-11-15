// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2005 Michael Noland (Joat)
// Copyright (C) 2005 Jason Rogers (Dovoto)
// Copyright (C) 2005 Dave Murphy (WinterMute)
// Copyright (C) 2023 Antonio Niño Díaz

#include <time.h>

#include <nds/arm7/clock.h>
#include <nds/bios.h>
#include <nds/interrupts.h>
#include <nds/ipc.h>
#include <nds/timers.h>
#include <nds/system.h>

#include "common/libnds_internal.h"

// Delay (in swiDelay units) for each bit transfer
#define RTC_DELAY 48

// Pin defines on RTC_CR
#define CS_0    (1 << 6)
#define CS_1    ((1 << 6) | (1 << 2))
#define SCK_0   (1 << 5)
#define SCK_1   ((1 << 5) | (1 << 1))
#define SIO_0   (1 << 4)
#define SIO_1   ((1 << 4) | (1 << 0))
#define SIO_out (1 << 4)
#define SIO_in  (1)

void BCDToInteger(uint8_t *data, uint32_t length)
{
    for (u32 i = 0; i < length; i++)
        data[i] = (data[i] & 0xF) + ((data[i] & 0xF0) >> 4) * 10;
}

void integerToBCD(uint8_t *data, uint32_t length)
{
    for (u32 i = 0; i < length; i++)
    {
        int high, low;
        swiDivMod(data[i], 10, &high, &low);
        data[i] = (high << 4) | low;
    }
}

void rtcTransaction(uint8_t *command, uint32_t commandLength, uint8_t *result,
                    uint32_t resultLength)
{
    uint32_t bit;
    uint8_t data;

    // Raise CS
    RTC_CR8 = CS_0 | SCK_1 | SIO_1;
    swiDelay(RTC_DELAY);
    RTC_CR8 = CS_1 | SCK_1 | SIO_1;
    swiDelay(RTC_DELAY);

    // Write command byte (high bit first)
    data = *command++;

    for (bit = 0; bit < 8; bit++)
    {
        RTC_CR8 = CS_1 | SCK_0 | SIO_out | (data >> 7);
        swiDelay(RTC_DELAY);

        RTC_CR8 = CS_1 | SCK_1 | SIO_out | (data >> 7);
        swiDelay(RTC_DELAY);

        data = data << 1;
    }

    // Write parameter bytes (low bit first)
    for (; commandLength > 1; commandLength--)
    {
        data = *command++;

        for (bit = 0; bit < 8; bit++)
        {
            RTC_CR8 = CS_1 | SCK_0 | SIO_out | (data & 1);
            swiDelay(RTC_DELAY);

            RTC_CR8 = CS_1 | SCK_1 | SIO_out | (data & 1);
            swiDelay(RTC_DELAY);

            data = data >> 1;
        }
    }

    // Read result bytes (low bit first)
    for (; resultLength > 0; resultLength--)
    {
        data = 0;

        for (bit = 0; bit < 8; bit++)
        {
            RTC_CR8 = CS_1 | SCK_0;
            swiDelay(RTC_DELAY);

            RTC_CR8 = CS_1 | SCK_1;
            swiDelay(RTC_DELAY);

            if (RTC_CR8 & SIO_in)
                data |= (1 << bit);
        }
        *result++ = data;
    }

    // Finish up by dropping CS low
    RTC_CR8 = CS_0 | SCK_1;
    swiDelay(RTC_DELAY);
}

void rtcReset(void)
{
    uint8_t status;
    uint8_t command[2];

    // Read the first status register
    command[0] = READ_STATUS_REG1;
    rtcTransaction(command, 1, &status, 1);

    // Reset the RTC if needed
    if (status & (STATUS_POC | STATUS_BLD))
    {
        command[0] = WRITE_STATUS_REG1;
        command[1] = status | STATUS_RESET;
        rtcTransaction(command, 2, 0, 0);
    }
}

__attribute__((deprecated)) void rtcGetTimeAndDate(uint8_t *time)
{
    uint8_t command, status;

    command = READ_TIME_AND_DATE;
    rtcTransaction(&command, 1, time, 7);

    command = READ_STATUS_REG1;
    rtcTransaction(&command, 1, &status, 1);

    if (status & STATUS_24HRS)
        time[4] &= 0x3f;

    BCDToInteger(time, 7);
}

__attribute__((deprecated)) void rtcSetTimeAndDate(uint8_t *time)
{
    uint8_t command[8];

    for (int i = 0; i < 7; i++)
        command[i + 1] = time[i];

    command[0] = WRITE_TIME_AND_DATE;

    // FIXME: range checking on the data we tell it
    rtcTransaction(command, 8, 0, 0);
}

__attribute__((deprecated)) void rtcGetTime(uint8_t *time)
{
    uint8_t command, status;

    command = READ_TIME;
    rtcTransaction(&command, 1, time, 3);

    command = READ_STATUS_REG1;
    rtcTransaction(&command, 1, &status, 1);
    if (status & STATUS_24HRS)
        time[0] &= 0x3f;

    BCDToInteger(time, 3);
}

__attribute__((deprecated)) void rtcSetTime(uint8_t *time)
{
    uint8_t command[4];

    for (int i = 0; i < 3; i++)
        command[i + 1] = time[i];

    command[0] = WRITE_TIME;

    // FIXME: range checking on the data we tell it
    rtcTransaction(command, 4, 0, 0);
}

static void syncRTC(void)
{
    __transferRegion()->unixTime++;
}

/* Nonzero if `y' is a leap year, else zero. */
#define leap(y) (((y) % 4 == 0 && (y) % 100 != 0) || (y) % 400 == 0)

/* Number of leap years from 1970 to `y' (not including `y' itself). */
#define nleap(y) (((y)-1969) / 4 - ((y)-1901) / 100 + ((y)-1601) / 400)

/* Additional leapday in February of leap years. */
#define leapday(m, y) ((m) == 1 && leap(y))

/* Accumulated number of days from 01-Jan up to start of current month. */
static const short ydays[] = {
    0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365
};

/* Length of month `m' (0 .. 11) */
#define monthlen(m, y) (ydays[(m) + 1] - ydays[m] + leapday(m, y))

static time_t __mktime(rtcTimeAndDate *dstime)
{
    int years, months, days, hours, minutes, seconds;

    years = dstime->year + 2000; /* year - 2000 -> year */
    months = dstime->month - 1;  /* 0..11 */
    days = dstime->day - 1;      /* 1..31 -> 0..30 */
    hours = dstime->hours;       /* 0..23 */
    minutes = dstime->minutes;   /* 0..59 */
    seconds = dstime->seconds;   /* 0..61 in ANSI C. */

    /* Set `days' to the number of days into the year. */
    days += ydays[months] + (months > 1 && leap(years));

    /* Now calculate `days' to the number of days since Jan 1, 1970. */
    days = (unsigned)days + 365 * (unsigned)(years - 1970) + (unsigned)(nleap(years));

    return (time_t)(86400L * (unsigned long)days + 3600L * (unsigned long)hours
                    + (unsigned long)(60 * minutes + seconds));
}

static bool __is_valid_time_date(rtcTimeAndDate *rtc)
{
    if (rtc->year > 99)
        return false;

    if ((rtc->month < 1) || (rtc->month > 12))
        return false;

    const uint8_t numdays[12] = {
        31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
    };
    uint32_t days = numdays[rtc->month - 1];

    if ((rtc->month == 2) && leap(2000 + (uint32_t)rtc->year))
        days++;

    if ((rtc->day < 1) || (rtc->day > days))
        return false;

    // TODO: Check weekday? Is this even used by the firmware or games?

    if (rtc->hours > 23)
        return false;

    if (rtc->minutes > 59)
        return false;

    if (rtc->seconds > 59)
        return false;

    return true;
}

static bool __is_valid_time(rtcTime *rtc)
{
    if (rtc->hours > 23)
        return false;

    if (rtc->minutes > 59)
        return false;

    if (rtc->seconds > 59)
        return false;

    return true;
}

void rtcTimeGet(rtcTime *rtc)
{
    uint8_t command, status, response[3];

    command = READ_TIME;
    rtcTransaction(&command, 1, response, 3);

    command = READ_STATUS_REG1;
    rtcTransaction(&command, 1, &status, 1);

    if (status & STATUS_24HRS)
        response[0] &= 0x3f;

    BCDToInteger(response, 3);

    rtc->hours = response[0];
    rtc->minutes = response[1];
    rtc->seconds = response[2];
}

int rtcTimeSet(rtcTime *rtc)
{
    if (!__is_valid_time(rtc))
        return -1;

    uint8_t command[4] = {
        WRITE_TIME,
        rtc->hours,
        rtc->minutes,
        rtc->seconds
    };

    integerToBCD(&command[1], 3);

    rtcTransaction(command, 4, 0, 0);

    return 0;
}

void rtcTimeAndDateGet(rtcTimeAndDate *rtc)
{
    uint8_t command, status, response[7];

    command = READ_TIME_AND_DATE;
    rtcTransaction(&command, 1, response, 7);

    command = READ_STATUS_REG1;
    rtcTransaction(&command, 1, &status, 1);

    if (status & STATUS_24HRS)
        response[4] &= 0x3f;

    BCDToInteger(response, 7);

    rtc->year = response[0];
    rtc->month = response[1];
    rtc->day = response[2];
    rtc->weekday = response[3];
    rtc->hours = response[4];
    rtc->minutes = response[5];
    rtc->seconds = response[6];
}

int rtcTimeAndDateSet(rtcTimeAndDate *rtc)
{
    if (!__is_valid_time_date(rtc))
        return -1;

    uint8_t command[8] = {
        WRITE_TIME_AND_DATE,
        rtc->year,
        rtc->month,
        rtc->day,
        rtc->weekday,
        rtc->hours,
        rtc->minutes,
        rtc->seconds
    };

    integerToBCD(&command[1], 7);

    rtcTransaction(command, 8, 0, 0);

    return 0;
}

void resyncClock(void)
{
    rtcTimeAndDate dstime;
    rtcTimeAndDateGet(&dstime);

    __transferRegion()->unixTime = __mktime(&dstime);
}

__attribute__((deprecated)) void initClockIRQ(void)
{
    REG_RCNT = 0x8100;
    irqSet(IRQ_RTC, syncRTC);

    // Reset the clock if needed
    rtcReset();

    uint8_t command[4];
    command[0] = READ_STATUS_REG2;
    rtcTransaction(command, 1, &command[1], 1);

    command[0] = WRITE_STATUS_REG2;
    command[1] = STATUS_INT2AE | STATUS_INT1FE;
    rtcTransaction(command, 2, 0, 0);

    command[0] = WRITE_INT_REG1;
    command[1] = 0x01;
    rtcTransaction(command, 2, 0, 0);

    command[0] = WRITE_INT_REG2;
    command[1] = 0x00;
    command[2] = 0x21;
    command[3] = 0x35;
    rtcTransaction(command, 4, 0, 0);

    // Read all time settings on first start
    resyncClock();
}

void initClockIRQTimer(int timer)
{
    // Reset the clock if needed
    rtcReset();

    // Read all time settings on first start
    resyncClock();

    // Setup a timer that triggers an interrupt once per second
    timerStart(timer, ClockDivider_1024, TIMER_FREQ_1024(1), syncRTC);
}
