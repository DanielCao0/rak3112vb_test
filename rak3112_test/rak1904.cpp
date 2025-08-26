/**
 * @file rak1904.cpp
 * @author RAK1904 Accelerometer Module Test
 * @brief RAK1904 3-axis accelerometer sensor (LIS3DH) implementation
 * @date 2025-08-25
 */

#include "rak1904.h"
#include "utilities.h"
#include "command.h"
#include <Arduino.h>
#include <Wire.h>
#include "SparkFunLIS3DH.h" //http://librarymanager/All#SparkFun-LIS3DH



LIS3DH rak1904(I2C_MODE, 0x18);

// 添加传感器状态标志
bool rak1904_initialized = false;

/**
 * @brief Initialize RAK1904 accelerometer sensor
 */
void init_rak1904() {
    Serial.println("Initializing RAK1904 accelerometer...");
    
    // // Initialize I2C
    // Wire.begin(I2C_SDA, I2C_SCL);

    // Register only the test AT command
    register_at_handler("AT+ACC", handle_at_rak1904_test, "Run RAK1904 self test");
    
    Serial.println("RAK1904 AT command registered");

    if (rak1904.begin() != 0)
	{
		Serial.println("Problem starting the sensor at 0x18.");
		rak1904_initialized = false;  // 标记初始化失败
	}
	else
	{
		Serial.println("Sensor at 0x18 started.");
		rak1904_initialized = true;   // 标记初始化成功
		// Set low power mode
		uint8_t data_to_write = 0;
		rak1904.readRegister(&data_to_write, LIS3DH_CTRL_REG1);
		data_to_write |= 0x08;
		rak1904.writeRegister(LIS3DH_CTRL_REG1, data_to_write);
		delay(100);

		data_to_write = 0;
		rak1904.readRegister(&data_to_write, 0x1E);
		data_to_write |= 0x90;
		rak1904.writeRegister(0x1E, data_to_write);
		delay(100);
	}
}


void lis3dh_read_data()
{
  // 检查传感器是否初始化成功
  if (!rak1904_initialized) {
    Serial.println("Sensor not initialized");
    return;
  }
  
  // read the sensor value
  uint8_t cnt = 0;

  Serial.print("X(g) = ");
  Serial.println(rak1904.readFloatAccelX(), 4);
  Serial.print("Y(g) = ");
  Serial.println(rak1904.readFloatAccelY(), 4);
  Serial.print("Z(g)= ");
  Serial.println(rak1904.readFloatAccelZ(), 4);
}

// AT Command Handler
void handle_at_rak1904_test(const AT_Command *cmd) {
    // 如果传感器初始化失败，返回0
    if (!rak1904_initialized) {
        Serial.println("0");  // 返回0表示传感器不可用
        return;
    }
    
    // 传感器正常，读取数据
    lis3dh_read_data();
    Serial.println("OK");
}

