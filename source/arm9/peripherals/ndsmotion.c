// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2007 Michael Noland (joat)
// Copyright (C) 2007 Jason Rogers (dovoto)
// Copyright (C) 2007 Dave Murphy (WinterMute)
// Copyright (C) 2007 Keith Epstein (KeithE)

// DS Motion Card/DS Motion Pak functionality

#include <nds/arm9/ndsmotion.h>
#include <nds/bios.h>
#include <nds/card.h>
#include <nds/memory.h>
#include <nds/system.h>

#define WAIT_CYCLES 185

static inline void CARD_WaitBusy(void)
{
    while (REG_AUXSPICNT & /*BUSY*/ 0x80)
        ;
}

// Enables SPI bus at 4.19 MHz
static inline void SPI_On(void)
{
    REG_AUXSPICNT = /*E*/ 0x8000 | /*SEL*/ 0x2000 | /*MODE*/ 0x40 | 0;
}

// Disables SPI bus
static inline void SPI_Off(void)
{
    REG_AUXSPICNT = 0;
}

// Volatile GBA bus SRAM for reading from DS Motion Pak
#define V_SRAM ((volatile unsigned char *)0x0A000000)

int card_type = -1;

// these are the default calibration values for sensitivity and offset
MotionCalibration calibration = { 2048, 2048, 2048, 1680, 819, 819, 819, 825 };

// sends and receives 1 byte on the SPI bus
unsigned char motion_spi(unsigned char in_byte)
{
    unsigned char out_byte;
    REG_AUXSPIDATA = in_byte;  // Send the output byte to the SPI bus
    CARD_WaitBusy();           // Wait for transmission to complete
    out_byte = REG_AUXSPIDATA; // Read the input byte from the SPI bus
    return out_byte;
}

void motion_MK6_sensor_mode(void)
{
    // Send some commands on the SPI bus
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
    motion_spi(0xF8);
    SPI_Off();
}

void motion_MK6_EEPROM_mode(void)
{
    // Send some commands on the SPI bus
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
    motion_spi(0xF9);
    SPI_Off();
}

// Checks whether a DS Motion Pak is plugged in
int motion_pak_is_inserted(void)
{
    int motion_pak = 0;

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
            motion_pak = 1;
    }

    return motion_pak;
}

// Checks whether a DS Motion Card is plugged in. This only works after
// motion_init(). It will return false if it is run before motion_init().
int motion_card_is_inserted(void)
{
    // Send 0x03 to read from DS Motion Card control register
    SPI_On();
    motion_spi(0x03); // Command to read from control register

    // If the control register is 0x04 then the enable was successful
    if (motion_spi(0x00) == 0x04)
    {
        SPI_Off();
        return 1;
    }

    SPI_Off();
    return 0;
}

// urn on the DS Motion Sensor (DS Motion Pak or DS Motion Card)
// Requires knowing which type is present (can be found by using motion_init)
int motion_enable(int card_type)
{
    switch (card_type)
    {
        case 1: // DS Motion Pak - automatically enabled on powerup
            // Check to see whether Motion Pak is alive
            return motion_pak_is_inserted();

        case 2: // DS Motion Card
            // Send 0x04, 0x04 to enable
            SPI_On();
            motion_spi(0x04); // Command to write to control register
            motion_spi(0x04); // Enable
            SPI_Off();
            // check to see whether Motion Card is alive
            return motion_card_is_inserted();

        case 3: // MK6 - same command as DS Motion Card
            // Send 0x04, 0x04 to enable
            SPI_On();
            motion_spi(0x04); // Command to write to control register
            motion_spi(0x04); // Enable
            SPI_Off();
            // Check to see whether Motion Card is alive
            return motion_card_is_inserted();

        default: // If input parameter is not recognized, return 0
            return 0;
    }
}

// Initialize the DS Motion Sensor. Determines which DS Motion Sensor is
// present and turns it on. It does not require knowing which type is present.
int motion_init(void)
{
    sysSetBusOwners(BUS_OWNER_ARM9, BUS_OWNER_ARM9);

    // First, check for the DS Motion Pak - type 1
    if (motion_pak_is_inserted() == 1)
    {
        card_type = 1;
        return 1;
    }

    // Next, check for DS Motion Card - type 2
    if (motion_enable(2) == 1)
    {
        card_type = 2;
        return 2;
    }

    // Send command to switch MK6 to sensor mode
    motion_MK6_sensor_mode();

    if (motion_enable(3) == 1)
    {
        card_type = 3;
        return 3;
    }

    // If neither cases are true, then return 0 to indicate no DS Motion Sensor
    return 0;
}

// Deinitialize the DS Motion Sensor.
// - In the case of a DS Motion Pak, do nothing - there is nothing to de-init.
// - In the case of a DS Motion Card, turns off the accelerometer.
// - In the case of an MK6, turns off accelerometer and switches out of sensor
//   mode into EEPROM mode.
void motion_deinit(void)
{
    // DS Motion Card - turn off accelerometer
    SPI_On();
    motion_spi(0x04); // Command to write to control register
    motion_spi(0x00); // Turn it off
    SPI_Off();
    // MK6 - switch to EEPROM mode
    motion_MK6_EEPROM_mode(); // Switch MK6 to EEPROM mode
}

// read the X acceleration
signed int motion_read_x(void)
{
    unsigned char High_byte = 0;
    unsigned char Low_byte = 0;
    signed int output = 0;
    switch (card_type)
    {
        case 1:                    // DS Motion Pak
            High_byte = V_SRAM[2]; // Command to load X High onto bus
            swiDelay(WAIT_CYCLES); // Wait for data ready
            High_byte = V_SRAM[0]; // Get the high byte
            swiDelay(WAIT_CYCLES); // Wait for data ready
            Low_byte = V_SRAM[0];  // Get the low byte
            // Wait after for Motion Pak to be ready for the next command
            swiDelay(WAIT_CYCLES);
            output = (signed int)((High_byte << 8 | Low_byte) >> 4);
            return output;

        case 2: // DS Motion Card
            SPI_On();
            motion_spi(0x00); // Command to convert X axis
            swiDelay(625);    // Wait at least 40 microseconds for the A-D conversion
            // Read 16 bits and store as a 12 bit number
            output = ((motion_spi(0x00) << 8) | motion_spi(0x00)) >> 4;
            SPI_Off();
            return output;

        case 3: // MK6 - same command as DS Motion Card
            SPI_On();
            motion_spi(0x00); // Command to convert X axis
            swiDelay(625);    // Wait at least 40 microseconds for the A-D conversion
            // Read 16 bits and store as a 12 bit number
            output = ((motion_spi(0x00) << 8) | motion_spi(0x00)) >> 4;
            SPI_Off();
            return output;

        default:
            return 0;
    }
}

// read the Y acceleration
signed int motion_read_y(void)
{
    unsigned char High_byte = 0;
    unsigned char Low_byte = 0;
    signed int output = 0;

    switch (card_type)
    {
        case 1:                    // DS Motion Pak
            High_byte = V_SRAM[4]; // Command to load Y High onto bus
            swiDelay(WAIT_CYCLES); // Wait for data ready
            High_byte = V_SRAM[0]; // Get the high byte
            swiDelay(WAIT_CYCLES); // Wait for data ready
            Low_byte = V_SRAM[0];  // Get the low byte

            // Wait after for Motion Pak to be ready for next command
            swiDelay(WAIT_CYCLES);
            output = (signed int)((High_byte << 8 | Low_byte) >> 4);
            return output;

        case 2: // DS Motion Card
            SPI_On();
            motion_spi(0x02); // Command to convert Y axis
            swiDelay(625);    // Wait at least 40 microseconds for the A-D conversion
            // Read 16 bits and store as a 12 bit number
            output = ((motion_spi(0x00) << 8) | motion_spi(0x00)) >> 4;
            SPI_Off();
            return output;

        case 3: // MK6 - same command as DS Motion Card
            SPI_On();
            motion_spi(0x02); // Command to convert Y axis
            swiDelay(625);    // Wait at least 40 microseconds for the A-D conversion
            // Read 16 bits and store as a 12 bit number
            output = ((motion_spi(0x00) << 8) | motion_spi(0x00)) >> 4;
            SPI_Off();
            return output;

        default:
            return 0;
    }
}

// read the Z acceleration
signed int motion_read_z(void)
{
    unsigned char High_byte = 0;
    unsigned char Low_byte = 0;
    signed int output = 0;

    switch (card_type)
    {
        case 1: // DS Motion Pak
            High_byte = V_SRAM[6]; // Command to load Z High onto bus
            swiDelay(WAIT_CYCLES); // Wait for data ready
            High_byte = V_SRAM[0]; // Get the high byte
            swiDelay(WAIT_CYCLES); // Wait for data ready
            Low_byte = V_SRAM[0];  // Get the low byte
            // Wait after for Motion Pak to be ready for next command
            swiDelay(WAIT_CYCLES);
            output = (signed int)((High_byte << 8 | Low_byte) >> 4);
            return output;

        case 2: // DS Motion Card
            SPI_On();
            motion_spi(0x01); // Command to convert Z axis
            swiDelay(625);    // Wait at least 40 microseconds for the A-D conversion
            // Read 16 bits and store as a 12 bit number
            output = ((motion_spi(0x00) << 8) | motion_spi(0x00)) >> 4;
            SPI_Off();
            return output;

        case 3: // MK6 - same command as DS Motion Card
            SPI_On();
            motion_spi(0x01); // Command to convert Z axis
            swiDelay(625);    // Wait at least 40 microseconds for the A-D conversion
            // Read 16 bits and store as a 12 bit number
            output = ((motion_spi(0x00) << 8) | motion_spi(0x00)) >> 4;
            SPI_Off();
            return output;

        default:
            return 0;
    }
}

// read the Z rotation (gyro)
signed int motion_read_gyro(void)
{
    unsigned char High_byte = 0;
    unsigned char Low_byte = 0;
    signed int output = 0;

    switch (card_type)
    {
        case 1: // DS Motion Pak
            High_byte = V_SRAM[8]; // Command to load Gyro High onto bus
            swiDelay(WAIT_CYCLES); // Wait for data ready
            High_byte = V_SRAM[0]; // Get the high byte
            swiDelay(WAIT_CYCLES); // Wait for data ready
            Low_byte = V_SRAM[0];  // Get the low byte
            // Wait after for Motion Pak to be ready for next command
            swiDelay(WAIT_CYCLES);
            output = (signed int)((High_byte << 8 | Low_byte) >> 4);
            return output;

        case 2: // DS Motion Card
            SPI_On();
            motion_spi(0x07); // Command to convert Gyro axis
            swiDelay(625);    // Wait at least 40 microseconds for the A-D conversion
            // Read 16 bits and store as a 12 bit number
            output = ((motion_spi(0x00) << 8) | motion_spi(0x00)) >> 4;
            SPI_Off();
            return output;

        case 3: // MK6 - same command as DS Motion Card
            SPI_On();
            motion_spi(0x07); // Command to convert Gyro axis
            swiDelay(625);    // Wait at least 40 microseconds for the A-D conversion
            // Read 16 bits and store as a 12 bit number
            output = ((motion_spi(0x00) << 8) | motion_spi(0x00)) >> 4;
            SPI_Off();
            return output;

        default:
            return 0;
    }
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
    V_SRAM[16];
    swiDelay(WAIT_CYCLES);
}

// enable analog input number 2 (ain_2)
void motion_enable_ain_2(void)
{
    V_SRAM[18];
    swiDelay(WAIT_CYCLES);
}

// read from the analog input number 1 - requires enabling ain_1 first
int motion_read_ain_1(void)
{
    unsigned char High_byte = V_SRAM[12]; // Command to load AIN_1 High onto bus
    swiDelay(WAIT_CYCLES);                // Wait for data ready
    High_byte = V_SRAM[0];                // Get the high byte
    swiDelay(WAIT_CYCLES);                // Wait for data ready
    unsigned char Low_byte = V_SRAM[0];   // Get the low byte
    // Wait after for Motion Pak to be ready for next command
    swiDelay(WAIT_CYCLES);
    signed int output = (signed int)((High_byte << 8 | Low_byte) >> 4);
    return output;
}

// read from the analog input number 2 - requires enabling ain_2 first
int motion_read_ain_2(void)
{
    unsigned char High_byte = V_SRAM[14]; // Command to load AIN_1 High onto bus
    swiDelay(WAIT_CYCLES);                // Wait for data ready
    High_byte = V_SRAM[0];                // Get the high byte
    swiDelay(WAIT_CYCLES);                // Wait for data ready
    unsigned char Low_byte = V_SRAM[0];   // Get the low byte
    // Wait after for Motion Pak to be ready for next command
    swiDelay(WAIT_CYCLES);
    signed int output = (signed int)((High_byte << 8 | Low_byte) >> 4);
    return output;
}
