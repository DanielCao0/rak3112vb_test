/**
 * @file rak1904.h
 * @author RAK1904 Accelerometer Module Test
 * @brief RAK1904 3-axis accelerometer sensor (LIS3DH) driver and test functions
 * @date 2025-08-25
 */

#ifndef RAK1904_H
#define RAK1904_H

#include <Arduino.h>
#include <Wire.h>
#include "command.h"
// Note: Install SparkFun LIS3DH Arduino Library from Library Manager
// #include <SparkFunLIS3DH.h>  // Uncomment after installing library

// RAK1904 I2C Address
#define RAK1904_I2C_ADDR    0x18

// Global variables
extern bool rak1904_initialized;
// extern LIS3DH rak1904_sensor;  // Uncomment after installing SparkFunLIS3DH library

// Function declarations
void init_rak1904();
void rak1904_self_test();

// AT command handler
void handle_at_rak1904_test(const AT_Command *cmd);

#endif // RAK1904_H
