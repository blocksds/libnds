// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2005 Michael Noland (Joat)
// Copyright (C) 2005 Jason Rogers (Dovoto)
// Copyright (C) 2005 Dave Murphy (WinterMute)

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

void rtcGetTimeAndDate(uint8_t *time)
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

void rtcSetTimeAndDate(uint8_t *time)
{
    uint8_t command[8];

    for (int i = 0; i < 7; i++)
        command[i + 1] = time[i];

    command[0] = WRITE_TIME_AND_DATE;

    // FIXME: range checking on the data we tell it
    rtcTransaction(command, 8, 0, 0);
}

void rtcGetTime(uint8_t *time)
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

void rtcSetTime(uint8_t *time)
{
    uint8_t command[4];

    for (int i = 0; i < 3; i++)
        command[i + 1] = time[i];

    command[0] = WRITE_TIME;

    // FIXME: range checking on the data we tell it
    rtcTransaction(command, 4, 0, 0);
}

void syncRTC(void)
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

static time_t __mktime(RTCtime *dstime)
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

void resyncClock(void)
{
    RTCtime dstime;
    rtcGetTimeAndDate((uint8_t *)&dstime);

    __transferRegion()->unixTime = __mktime(&dstime);
}

static void initClockInternal(void)
{
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

void initClockIRQ(void)
{
    REG_RCNT = 0x8100;
    irqSet(IRQ_RTC, syncRTC);

    initClockInternal();
}

void initClockIRQTimer(int timer)
{
    // Setup a timer that triggers an interrupt once per second
    timerStart(timer, ClockDivider_1024, TIMER_FREQ_1024(1), syncRTC);

    initClockInternal();
}
