/*
   RadioLib Transmit with Interrupts Example

   This example transmits packets using SX1276/SX1278/SX1262/SX1268/SX1280/LR1121 LoRa radio module.
   Each packet contains up to 256 bytes of data, in the form of:
    - Arduino String
    - null-terminated char array (C-string)
    - arbitrary binary data (byte array)

   For full API reference, see the GitHub Pages
   https://jgromes.github.io/RadioLib/
*/
#include <RadioLib.h>
#include "utilities.h"
#include "WiFi.h"
#include "command.h"
#include "lora.h"
#include "ble.h"



void wifiScan(void);
void handle_at_wifiscan(const AT_Command *cmd)
{
  wifiScan();
}

void setupBoards(void)
{
  Serial.begin(115200);

  Serial.println("setupBoards");

  // RF Switch enable pin
  pinMode(4, OUTPUT);
  digitalWrite(4, HIGH);

  SPI.begin(RADIO_SCLK_PIN, RADIO_MISO_PIN, RADIO_MOSI_PIN);

  Serial.println("init done .");

  // Set WiFi to station mode and disconnect from an AP if it was previously connected.
  // WiFi.mode(WIFI_STA);
  // WiFi.disconnect();
  // delay(100);
  // register_at_handler("AT+WIFISCAN", handle_at_wifiscan, "Scan WiFi networks");
}

void setup()
{
  setupBoards();
  init_lora_radio();
  command_init();
  ble_init();
}

void loop()
{
  delay(10);
}

void wifiScan()
{
  Serial.println("Scan start");

  // WiFi.scanNetworks will return the number of networks found.
  int n = WiFi.scanNetworks();
  Serial.println("Scan done");
  if (n == 0)
  {
    Serial.println("no networks found");
  }
  else
  {
    Serial.print(n);
    Serial.println(" networks found");
    Serial.println("Nr | SSID                             | RSSI | CH | Encryption");
    for (int i = 0; i < n; ++i)
    {
      // Print SSID and RSSI for each network found
      Serial.printf("%2d", i + 1);
      Serial.print(" | ");
      Serial.printf("%-32.32s", WiFi.SSID(i).c_str());
      Serial.print(" | ");
      Serial.printf("%4ld", WiFi.RSSI(i));
      Serial.print(" | ");
      Serial.printf("%2ld", WiFi.channel(i));
      Serial.print(" | ");
      switch (WiFi.encryptionType(i))
      {
      case WIFI_AUTH_OPEN:
        Serial.print("open");
        break;
      case WIFI_AUTH_WEP:
        Serial.print("WEP");
        break;
      case WIFI_AUTH_WPA_PSK:
        Serial.print("WPA");
        break;
      case WIFI_AUTH_WPA2_PSK:
        Serial.print("WPA2");
        break;
      case WIFI_AUTH_WPA_WPA2_PSK:
        Serial.print("WPA+WPA2");
        break;
      case WIFI_AUTH_WPA2_ENTERPRISE:
        Serial.print("WPA2-EAP");
        break;
      case WIFI_AUTH_WPA3_PSK:
        Serial.print("WPA3");
        break;
      case WIFI_AUTH_WPA2_WPA3_PSK:
        Serial.print("WPA2+WPA3");
        break;
      case WIFI_AUTH_WAPI_PSK:
        Serial.print("WAPI");
        break;
      default:
        Serial.print("unknown");
      }
      Serial.println();
      delay(10);
    }
  }
  Serial.println("");

  // Delete the scan result to free memory for code below.
  WiFi.scanDelete();

  // // Wait a bit before scanning again.
  // delay(5000);
}



