#ifndef __GPS_H__
#define __GPS_H__

#include "Arduino.h"
#include "command.h"

// RAK3112 GPS pins (adjust these according to your actual hardware)
#define GPS_POWER_PIN   14    // WB_IO2 equivalent 
#define GPS_TX_PIN      43   // GPS module TX pin (connect to ESP32 RX)
#define GPS_RX_PIN      44   // GPS module RX pin (connect to ESP32 TX)

//#define GPS_PRINTF
#ifdef GPS_PRINTF
  #define GPS_PRINT(x)        Serial.print(x)
  #define GPS_PRINTLN(x)      Serial.println(x)
  #define GPS_PRINT_WRITE(x)  Serial.write(x)
#else
  #define GPS_PRINT(x)
  #define GPS_PRINTLN(x)
  #define GPS_PRINT_WRITE(x)
#endif


#ifdef GPS_DEBUG
  #define GPS_PRINT_WRITE(x)  Serial.write(x)
#else
  #define GPS_PRINT_WRITE(x)
#endif

struct GPSData {
  int32_t lon;   // 4Bytes GNSS Floor(Longitude * 1_000_000)
  int32_t lat;   // 4Bytes GNSS Floor(Latitude * 1_000_000)
  uint8_t acc;   // 1Byte GNSS 0-255
  uint8_t sate;  // 1Byte
  uint32_t utc;  // 4Bytes UTC 
  uint8_t sta;   // 1Byte
};

extern GPSData gpsData;

void init_gps();
void gpsParseDate();
void gpsOn();
void gpsOff();
void handle_at_gpsget(const AT_Command *cmd);

#endif
