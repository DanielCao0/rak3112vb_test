#include "l76k.h"
#include "command.h"

bool verifyChecksum(String sentence);
int32_t convertToDecimalDegrees(String rawCoord, String direction);
void parseGNGGA(String sentence);

String nmeaSentence = "";
bool sentenceStarted = false;
GPSData gpsData;

void init_gps()
{
  pinMode(GPS_POWER_PIN, OUTPUT);
  digitalWrite(GPS_POWER_PIN, 0);
  delay(1000);
  digitalWrite(GPS_POWER_PIN, 1);
  delay(1000);
  
  // Initialize Serial1 with specific pins for ESP32S3
  Serial1.begin(9600, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
  register_at_handler("AT+GPS", handle_at_gpsget, "Get current GPS data");
  //Serial.println("[GPS] GPS module initialized with pins TX:" + String(GPS_RX_PIN) + " RX:" + String(GPS_TX_PIN));
}

void gpsParseDate()
{
  while (Serial1.available())
  {
    char c = Serial1.read();
    GPS_PRINT_WRITE(c);

    if (c == '$')
    {
      sentenceStarted = true;
      nmeaSentence = "";
    }

    if (sentenceStarted)
    {
      nmeaSentence += c;
      if (c == '\n' || c == '\r')
      {
        sentenceStarted = false;

        if (nmeaSentence.startsWith("$GNGGA"))
        {
          if (verifyChecksum(nmeaSentence))
          {
            parseGNGGA(nmeaSentence);
          }
          else
          {
            GPS_PRINTLN("[GPS] Checksum failed!");
          }
        }
      }
    }
  }
}

void gpsOn()
{
  digitalWrite(GPS_POWER_PIN, 1);
}

void gpsOff()
{
  digitalWrite(GPS_POWER_PIN, 0);
}

bool verifyChecksum(String sentence)
{
  int asteriskIndex = sentence.indexOf('*');
  if (asteriskIndex == -1 || asteriskIndex + 2 >= sentence.length())
    return false;

  byte checksum = 0;
  for (int i = 1; i < asteriskIndex; i++)
  {
    checksum ^= sentence.charAt(i);
  }

  String checksumStr = sentence.substring(asteriskIndex + 1, asteriskIndex + 3);
  byte receivedChecksum = strtol(checksumStr.c_str(), NULL, 16);

  return checksum == receivedChecksum;
}

int32_t convertToDecimalDegrees(String rawCoord, String direction)
{
  if (rawCoord.length() < 4)
    return 0;

  int pointIndex = rawCoord.indexOf('.');
  if (pointIndex == -1 || pointIndex < 2)
    return 0;

  int degrees = rawCoord.substring(0, pointIndex - 2).toInt();
  float minutes = rawCoord.substring(pointIndex - 2).toFloat();

  float decimal = degrees + minutes / 60.0;
  if (direction == "S" || direction == "W")
    decimal *= -1;

  return (int32_t)(decimal * 1000000);
}

void parseGNGGA(String sentence)
{
  sentence.trim();
  sentence.replace("\r", "");
  sentence.replace("\n", "");

  const int maxFields = 15;
  String fields[maxFields];
  int fieldIndex = 0;
  int lastIndex = 0;

  for (int i = 0; i < sentence.length() && fieldIndex < maxFields; i++)
  {
    if (sentence.charAt(i) == ',' || sentence.charAt(i) == '*')
    {
      fields[fieldIndex++] = sentence.substring(lastIndex, i);
      lastIndex = i + 1;
    }
  }

  if (fieldIndex < 13)
  {
    GPS_PRINTLN("[GPS] Incomplete GNGGA sentence.");
    return;
  }

  gpsData.utc = fields[1].toInt(); // UTC time (hhmmss)
  gpsData.lat = convertToDecimalDegrees(fields[2], fields[3]);
  gpsData.lon = convertToDecimalDegrees(fields[4], fields[5]);
  gpsData.sta = fields[6].toInt();                   // Fix quality
  gpsData.sate = fields[7].toInt();                  // Satellites
  gpsData.acc = (uint8_t)(fields[8].toFloat() * 10); // HDOP -> scale as needed

//  GPS_PRINTLN("==== Parsed GPS Data ====");
  GPS_PRINT("[GPS] UTC Time: ");
  GPS_PRINTLN(gpsData.utc);
  GPS_PRINT("[GPS] Latitude : ");
  GPS_PRINTLN(gpsData.lat);
  GPS_PRINT("[GPS] Longitude: ");
  GPS_PRINTLN(gpsData.lon);
  GPS_PRINT("[GPS] Fix Quality: ");
  GPS_PRINTLN(gpsData.sta);
  GPS_PRINT("[GPS] Satellites: ");
  GPS_PRINTLN(gpsData.sate);
  GPS_PRINT("[GPS] Accuracy: ");
  GPS_PRINTLN(gpsData.acc);
  GPS_PRINT("[GPS] Status: ");
  GPS_PRINTLN(gpsData.sta);
//  GPS_PRINTLN("=========================");
}

void handle_at_gpsget(const AT_Command *cmd)
{
  //Serial.println("==== Current GPS Data ====");
  Serial.print("UTC Time: ");
  Serial.println(gpsData.utc);
  Serial.print("Latitude : ");
  Serial.print((float)gpsData.lat / 1000000.0, 6);
  Serial.println(" degrees");
  Serial.print("Longitude: ");
  Serial.print((float)gpsData.lon / 1000000.0, 6);
  Serial.println(" degrees");
  Serial.print("Fix Quality: ");
  Serial.println(gpsData.sta);
  Serial.print("Satellites: ");
  Serial.println(gpsData.sate);
  Serial.print("Accuracy: ");
  Serial.print((float)gpsData.acc / 10.0, 1);
  Serial.println(" HDOP");
  Serial.println("OK");
 // Serial.println("==========================");
}
