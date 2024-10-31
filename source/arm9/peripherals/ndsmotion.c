// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2007 Michael Noland (joat)
// Copyright (C) 2007 Jason Rogers (dovoto)
// Copyright (C) 2007 Dave Murphy (WinterMute)
// Copyright (C) 2007 Keith Epstein (KeithE)
// Copyright (C) 2024 Adrian "asie" Siekierka

// DS Motion Card/DS Motion Pak functionality

#include <nds/arm9/ndsmotion.h>
#include <nds/bios.h>
#include <nds/card.h>
#include <nds/interrupts.h>
#include <nds/memory.h>
#include <nds/system.h>

#define WAIT_CYCLES 185

#define KXPB5_CMD_CONVERT_X     0x00
#define KXPB5_CMD_CONVERT_Z     0x01
#define KXPB5_CMD_CONVERT_Y     0x02
#define KXPB5_CMD_READ_CONTROL  0x03
#define KXPB5_CMD_WRITE_CONTROL 0x04
#define KXPB5_CMD_CONVERT_AUX   0x07

#define KXPB5_CONTROL_ENABLE    0x04
#define KXPB5_CONTROL_DISABLE   0x00
#define KXPB5_CONTROL_SELF_TEST 0x02
#define KXPB5_CONTROL_PARITY    0x01

// Enables SPI bus at 4.19 MHz
static inline void SPI_On(void)
{
    REG_AUXSPICNT = CARD_ENABLE | CARD_SPI_ENABLE | CARD_SPI_HOLD | CARD_SPI_BAUD_4MHz;
}

// Disables SPI bus
static inline void SPI_Off(void)
{
    REG_AUXSPICNT = 0;
}

// Volatile GBA bus SRAM for reading from DS Motion Pak
#define V_SRAM ((volatile unsigned char *)0x0A000000)

static uint8_t card_type = MOTION_TYPE_NONE;

// these are the default calibration values for sensitivity and offset
static MotionCalibration calibration = { 2048, 2048, 2048, 1680, 819, 819, 819, 825 };

// sends and receives 1 byte on the SPI bus
static unsigned char motion_spi(unsigned char in_byte)
{
    unsigned char out_byte;
    REG_AUXSPIDATA = in_byte;  // Send the output byte to the SPI bus
    eepromWaitBusy();           // Wait for transmission to complete
    out_byte = REG_AUXSPIDATA; // Read the input byte from the SPI bus
    return out_byte;
}

// MK6 helper functions
static void motion_MK6_command(uint8_t cmd)
{
    SPI_On();
    motion_spi(0xFE);
    SPI_Off();
    SPI_On();
    motion_spi(0xFD);
    SPI_Off();
    SPI_On();
    motion_spi(0xFB);
    SPI_Off();
    SPI_On();
    motion_spi(cmd);
    SPI_Off();
}

static void motion_MK6_sensor_mode(void)
{
    motion_MK6_command(0xF8);
}

static void motion_MK6_EEPROM_mode(void)
{
    motion_MK6_command(0xF9);
}

// ATTiny helper functions
enum {
    ATTINY_STEP_SYNC = 0,
    ATTINY_STEP_X,
    ATTINY_STEP_Y,
    ATTINY_STEP_Z
};

static uint8_t attiny_step = 0;

#define ATTINY_STEP_ERROR 0xFF
#define ATTINY_TIMEOUT 48

static uint8_t motion_attiny_read_bits(void)
{
    // TODO: Validate wait cycle count for the ATTiny cart.
    swiDelay(WAIT_CYCLES);
    return V_SRAM[0] & 3;
}

static uint8_t motion_attiny_read_value(void)
{
    uint8_t value = motion_attiny_read_bits();
    value = (value << 2) | motion_attiny_read_bits();
    value = (value << 2) | motion_attiny_read_bits();
    value = (value << 2) | motion_attiny_read_bits();
    return value;
}

static uint8_t motion_attiny_step(uint8_t target_step)
{
    uint8_t result = 0;

    target_step = (target_step + 1) & 3;
    while (attiny_step != target_step)
    {
        switch (attiny_step)
        {
            case ATTINY_STEP_SYNC:
                result = motion_attiny_read_bits();
                for (int i = 0; i < ATTINY_TIMEOUT; i++)
                {
                    if (result == 0)
                    {
                        if ((result = motion_attiny_read_bits()) != 3) continue;
                        if ((result = motion_attiny_read_bits()) != 3) continue;
                        if ((result = motion_attiny_read_bits()) != 3) continue;
                        if ((result = motion_attiny_read_bits()) != 3) continue;
                        result = 0;
                        attiny_step = ATTINY_STEP_X;
                        break;
                    }
                    result = motion_attiny_read_bits();
                }
                if (attiny_step == ATTINY_STEP_SYNC)
                    return ATTINY_STEP_ERROR;
                break;

            case ATTINY_STEP_X:
            case ATTINY_STEP_Y:
            case ATTINY_STEP_Z:
                result = motion_attiny_read_value();
                attiny_step = (attiny_step + 1) & 3;
                break;
        }
    }
    return result;
}

static bool motion_pak_attiny_is_inserted(void)
{
    if (isDSiMode())
        return false;

    if (*(vu16 *)0x08000000 != 0xFCFF)
        return false;

    return motion_attiny_step(ATTINY_STEP_SYNC) == 0;
}

// Checks whether a DS Motion Pak is plugged in
int motion_pak_is_inserted(void)
{
    if (isDSiMode())
        return 0;

    // Read first byte of DS Motion Pak check
    unsigned char return_byte = V_SRAM[10];
    swiDelay(WAIT_CYCLES);
    return_byte = V_SRAM[0];
    swiDelay(WAIT_CYCLES);

    if (return_byte == 0xF0) // DS Motion Pak returns 0xF0
    {
        return_byte = V_SRAM[0]; // Read second byte of DS Motion Pak check
        swiDelay(WAIT_CYCLES);
        if (return_byte == 0x0F) // DS Motion Pak returns 0x0F
            return 1;
    }

    return 0;
}

// Checks whether a DS Motion Card is plugged in. This only works after
// motion_init(). It will return false if it is run before motion_init().
int motion_card_is_inserted(void)
{
    // Send 0x03 to read from DS Motion Card control register
    SPI_On();
    motion_spi(KXPB5_CMD_READ_CONTROL); // Command to read from control register

    // If the control register is 0x04 then the enable was successful
    if (motion_spi(0x00) == KXPB5_CONTROL_ENABLE)
    {
        SPI_Off();
        return 1;
    }

    SPI_Off();
    return 0;
}

// Turn on the DS Motion Sensor (DS Motion Pak or DS Motion Card)
// Requires knowing which type is present (can be found by using motion_init)
static int motion_enable(int card_type_)
{
    switch (card_type_)
    {
        case MOTION_TYPE_PAK:
            // Check to see whether Motion Pak is alive
            return motion_pak_is_inserted();

        case MOTION_TYPE_CARD:
        case MOTION_TYPE_MK6:
            // Send 0x04, 0x04 to enable
            SPI_On();
            motion_spi(KXPB5_CMD_WRITE_CONTROL); // Command to write to control register
            motion_spi(KXPB5_CONTROL_ENABLE); // Enable
            SPI_Off();
            // check to see whether Motion Card is alive
            return motion_card_is_inserted();

        case MOTION_TYPE_PAK_ATTINY:
            return motion_pak_attiny_is_inserted();

        default: // If input parameter is not recognized, return 0
            return 0;
    }
}

extern void __libnds_twl_cardInit(void);

// Initialize the DS Motion Sensor. Determines which DS Motion Sensor is
// present and turns it on. It does not require knowing which type is present.
MotionType motion_init(void)
{
    sysSetBusOwners(BUS_OWNER_ARM9, BUS_OWNER_ARM9);

    // First, check for the DS Motion Pak
    if (motion_pak_is_inserted() == 1)
    {
        card_type = MOTION_TYPE_PAK;
        return MOTION_TYPE_PAK;
    }

    // Next, check for the ATTiny Motion Pack
    if (motion_pak_attiny_is_inserted())
    {
        card_type = MOTION_TYPE_PAK_ATTINY;
        return MOTION_TYPE_PAK_ATTINY;
    }

    if (isDSiMode())
        __libnds_twl_cardInit();

    // Next, check for DS Motion Card
    if (motion_enable(MOTION_TYPE_CARD) == 1)
    {
        card_type = MOTION_TYPE_CARD;
        return MOTION_TYPE_CARD;
    }

    // Send command to switch MK6 to sensor mode
    motion_MK6_sensor_mode();

    if (motion_enable(MOTION_TYPE_MK6) == 1)
    {
        card_type = MOTION_TYPE_MK6;
        return MOTION_TYPE_MK6;
    }

    // If neither cases are true, then return 0 to indicate no DS Motion Sensor
    return MOTION_TYPE_NONE;
}

MotionType motion_get_type(void)
{
    return card_type;
}

// Deinitialize the DS Motion Sensor.
// - In the case of a DS Motion Pak, do nothing - there is nothing to de-init.
// - In the case of a DS Motion Card, turns off the accelerometer.
// - In the case of an MK6, turns off accelerometer and switches out of sensor
//   mode into EEPROM mode.
void motion_deinit(void)
{
    if (card_type >= MOTION_TYPE_CARD)
    {
        // DS Motion Card - turn off accelerometer
        SPI_On();
        motion_spi(KXPB5_CMD_WRITE_CONTROL); // Command to write to control register
        motion_spi(KXPB5_CONTROL_DISABLE); // Turn it off
        SPI_Off();

        if (card_type == MOTION_TYPE_MK6)
            motion_MK6_EEPROM_mode(); // Switch MK6 to EEPROM mode
    }

    card_type = 0;
}

static const char *motion_names[] = {
    "None",
    "DS Motion Pak",
    "DS Motion Pack",
    "DS Motion Card",
    "MK6"
};

const char *motion_get_name(MotionType type)
{
    return motion_names[type > MOTION_TYPE_MK6 ? 0 : type];
}

static int motion_read(uint32_t pak_offset, uint8_t spi_command, int default_value)
{
    unsigned char High_byte = 0;
    unsigned char Low_byte = 0;
    int output = 0;

    switch (card_type)
    {
        case MOTION_TYPE_PAK:
            High_byte = V_SRAM[pak_offset];
            swiDelay(WAIT_CYCLES); // Wait for data ready
            High_byte = V_SRAM[0]; // Get the high byte
            swiDelay(WAIT_CYCLES); // Wait for data ready
            Low_byte = V_SRAM[0];  // Get the low byte
            // Wait after for Motion Pak to be ready for the next command
            swiDelay(WAIT_CYCLES);
            output = (int)((High_byte << 8 | Low_byte) >> 4);
            return output;

        case MOTION_TYPE_CARD:
        case MOTION_TYPE_MK6:
            SPI_On();
            motion_spi(spi_command);
            swiDelay(625);    // Wait at least 40 microseconds for the A-D conversion
            // Read 16 bits and store as a 12 bit number
            output = ((motion_spi(0x00) << 8) | motion_spi(0x00)) >> 4;
            SPI_Off();
            return output;

        case MOTION_TYPE_PAK_ATTINY:
            if (pak_offset <= 6)
            {
                High_byte = motion_attiny_step(pak_offset >> 1);
                if (High_byte != ATTINY_STEP_ERROR)
                    return (High_byte << 4);
            }
            // fall through

        default:
            return default_value;
    }
}

bool motion_accelerometer_supported(void)
{
    return card_type != MOTION_TYPE_NONE;
}

bool motion_gyroscope_supported(void)
{
    return card_type != MOTION_TYPE_NONE && card_type != MOTION_TYPE_PAK_ATTINY;
}

bool motion_ain_supported(void)
{
    return card_type == MOTION_TYPE_PAK;
}

// read the X acceleration
int motion_read_x(void)
{
    return motion_read(2, KXPB5_CMD_CONVERT_X, calibration.xoff);
}

// read the Y acceleration
int motion_read_y(void)
{
    return motion_read(4, KXPB5_CMD_CONVERT_Y, calibration.yoff);
}

// read the Z acceleration
int motion_read_z(void)
{
    return motion_read(6, KXPB5_CMD_CONVERT_Z, calibration.zoff);
}

// read the Z rotation (gyro)
int motion_read_gyro(void)
{
    return motion_read(8, KXPB5_CMD_CONVERT_AUX, calibration.goff);
}

// gets acceleration value in mili G (where g is 9.8 m/s*s)
int motion_acceleration_x(void)
{
    int accel = motion_read_x();
    return (accel - calibration.xoff) * 1000 / calibration.xsens;
}

// gets acceleration value in mili G (where g is 9.8 m/s*s)
int motion_acceleration_y(void)
{
    int accel = motion_read_y();
    return (accel - calibration.yoff) * 1000 / calibration.ysens;
}
// gets acceleration value in mili G (where g is 9.8 m/s*s)
int motion_acceleration_z(void)
{
    int accel = motion_read_z();
    return (accel - calibration.zoff) * 1000 / calibration.zsens;
}

// converts raw rotation value to degrees per second
int motion_rotation(void)
{
    int rotation = motion_read_gyro();
    return (rotation - calibration.goff) * 1000 / calibration.gsens;
}

// this should be passed the raw reading at 1g for accurate
// acceleration calculations.  Default is 819
void motion_set_sens_x(int sens)
{
    calibration.xsens = sens - calibration.xoff;
}

// this should be passed the raw reading at 1g for accurate
// acceleration calculations.  Default is 819
void motion_set_sens_y(int sens)
{
    calibration.ysens = sens - calibration.yoff;
}

// this should be passed the raw reading at 1g for accurate
// acceleration calculations.  Default is 819
void motion_set_sens_z(int sens)
{
    calibration.zsens = sens - calibration.zoff;
}

// this should be passed the raw reading at 1g for accurate
// acceleration calculations.  Default is 825
void motion_set_sens_gyro(int sens)
{
    calibration.gsens = sens;
}

// this should be called when the axis is under no acceleration
// default is 2048
void motion_set_offs_x(void)
{
    calibration.xoff = motion_read_x();
}

// this should be called when the axis is under no acceleration
// default is 2048
void motion_set_offs_y(void)
{
    calibration.yoff = motion_read_y();
}

// this should be called when the axis is under no acceleration
// default is 2048
void motion_set_offs_z(void)
{
    calibration.zoff = motion_read_z();
}

// this should be called when the axis is under no acceleration
// default is 1680
void motion_set_offs_gyro(void)
{
    calibration.goff = motion_read_gyro();
}

MotionCalibration *motion_get_calibration(void)
{
    return &calibration;
}

void motion_set_calibration(MotionCalibration *cal)
{
    calibration.xsens = cal->xsens;
    calibration.ysens = cal->ysens;
    calibration.zsens = cal->zsens;
    calibration.gsens = cal->gsens;
    calibration.xoff = cal->xoff;
    calibration.yoff = cal->yoff;
    calibration.zoff = cal->zoff;
    calibration.goff = cal->goff;
}

// enable analog input number 1 (ain_1)
void motion_enable_ain_1(void)
{
    if (card_type != MOTION_TYPE_PAK)
        return;

    V_SRAM[16];
    swiDelay(WAIT_CYCLES);
}

// enable analog input number 2 (ain_2)
void motion_enable_ain_2(void)
{
    if (card_type != MOTION_TYPE_PAK)
        return;

    V_SRAM[18];
    swiDelay(WAIT_CYCLES);
}

// read from the analog input number 1 - requires enabling ain_1 first
int motion_read_ain_1(void)
{
    if (card_type != MOTION_TYPE_PAK)
        return 0;

    return motion_read(12, 0, 0);
}

// read from the analog input number 2 - requires enabling ain_2 first
int motion_read_ain_2(void)
{
    if (card_type != MOTION_TYPE_PAK)
        return 0;

    return motion_read(14, 0, 0);
}

