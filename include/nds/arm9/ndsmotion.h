// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2007 Michael Noland (joat)
// Copyright (C) 2007 Jason Rogers (dovoto)
// Copyright (C) 2007 Dave Murphy (WinterMute)
// Copyright (C) 2007 Keith Epstein (KeithE)

/// @file nds/arm9/ndsmotion.h
///
/// @brief DS Motion Card/DS Motion Pak functionality
///
/// Interface code for the ds motion card, ds motion pak, MK6.

#ifndef LIBNDS_NDS_ARM9_NDSMOTION_H__
#define LIBNDS_NDS_ARM9_NDSMOTION_H__

typedef struct MotionCalibration {
    short xoff, yoff, zoff, goff;
    short xsens, ysens, zsens, gsens;
} MotionCalibration;

#ifdef __cplusplus
extern "C" {
#endif

/// Initializes the DS Motion Sensor.
///
/// Run this before using any of the DS Motion Sensor functions. Save the return
/// value and pass it to the other functions.
///
/// @return The motion sensor type, or 0 if there is no sensor present.
int motion_init(void);

/// Deinitializes the DS Motion Sensor.
void motion_deinit(void);

/// Reads the X acceleration.
///
/// @return The X acceleration.
signed int motion_read_x(void);

/// Reads the Y acceleration.
///
/// @return The Y acceleration.
signed int motion_read_y(void);

/// Reads the Z acceleration.
///
/// @return The Z acceleration.
signed int motion_read_z(void);

/// Reads the Z rotational speed.
///
/// @return The Z rotational speed.
signed int motion_read_gyro(void);

/// Gets acceleration value to mili G (where g is 9.8 m/s*s)
///
/// @return The X acceleration value.
int motion_acceleration_x(void);

/// Gets acceleration value to mili G (where g is 9.8 m/s*s)
///
/// @return The Y acceleration value.
int motion_acceleration_y(void);

/// Gets acceleration value to mili G (where g is 9.8 m/s*s)
///
/// @return The Z acceleration value.
int motion_acceleration_z(void);

/// This should be passed the raw reading at 1g for accurate acceleration
/// calculations.
///
/// Default is 819
///
/// @param sens The raw reading at 1g for accurate acceleration calculations.
void motion_set_sens_x(int sens);

/// This should be passed the raw reading at 1g for accurate acceleration
/// calculations.
///
/// Default is 819
///
/// @param sens The raw reading at 1g for accurate acceleration calculations.
void motion_set_sens_y(int sens);

/// This should be passed the raw reading at 1g for accurate acceleration
/// calculations.
///
/// Default is 819
///
/// @param sens The raw reading at 1g for accurate acceleration calculations.
void motion_set_sens_z(int sens);

/// This should be passed the raw reading at 1g for accurate acceleration
/// calculations.
///
/// Default is 825
///
/// @param sens The raw reading at 1g for accurate acceleration calculations.
void motion_set_sens_gyro(int sens);

/// This should be called when the axis is under no acceleration.
///
/// Default is 2048.
void motion_set_offs_x(void);

/// This should be called when the axis is under no acceleration.
///
/// Default is 2048.
void motion_set_offs_y(void);

/// This should be called when the axis is under no acceleration.
///
/// Default is 2048.
void motion_set_offs_z(void);

/// This should be called when the axis is under no rotation.
///
/// Default is 1680.
void motion_set_offs_gyro(void);

/// Converts raw rotation to degrees per second.
///
/// @return Degrees per second.
int motion_rotation(void);

/// This returns the current calibration settings for saving.
///
/// @return The calibration settings.
MotionCalibration *motion_get_calibration(void);

/// This sets the calibration settings.
///
/// Intended to restore previously saved calibration settings
///
/// @param cal The calibration settings
void motion_set_calibration(MotionCalibration *cal);

/// This enables the analog input number 1.
///
/// Required before reading analog input number 1.
void motion_enable_ain_1(void);

/// This enables the analog input number 2.
///
/// Required before reading analog input number 2.
void motion_enable_ain_2(void);

/// This reads the analog input number 1.
///
/// Analog input number 1 needs to be enabled before reading.
///
/// @return Analog input number 1.
int motion_read_ain_1(void);

/// This reads the analog input number 2.
///
/// Analog input number 2 needs to be enabled before reading.
///
/// @return Analog input number 2.
int motion_read_ain_2(void);

#ifdef __cplusplus
}
#endif

#endif // LIBNDS_NDS_ARM9_NDSMOTION_H__
