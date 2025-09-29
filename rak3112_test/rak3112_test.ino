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
#include "rak1904.h"
#include <U8g2lib.h>	
#include "l76k.h"
#include "lcd.h"    // 添加LCD支持
#include "sdcard.h" // 添加SD卡支持
#include <Adafruit_GFX.h>			// Click here to get the library: http://librarymanager/All#Adafruit_GFX
#include <Adafruit_ST7789.h>	// Click here to get the library: http://librarymanager/All#Adafruit_ST7789
#include <./Fonts/FreeSerif9pt7b.h>  // Font file, you can include your favorite fonts.
#include <SPI.h>


#define CS            12
#define BL            41
#define RST           -1
#define DC            42
#define BUZZER        38    // PWM蜂鸣器引脚

// Version information
#define FIRMWARE_VERSION "1.0.0"
#define BUILD_DATE __DATE__
#define BUILD_TIME __TIME__

U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0);

void check_button();

void init_rak1921()
{
  u8g2.begin();
  u8g2.clearBuffer();                    // clear the internal memory
  u8g2.setFont(u8g2_font_ncenB14_tr);    // choose a larger font
  u8g2.drawStr(0, 16, "RAK1921");  // write something to the display
  u8g2.sendBuffer();                     // transfer internal memory to the display
}

// 蜂鸣器初始化和响声函数
void init_buzzer()
{
  pinMode(BUZZER, OUTPUT);
  digitalWrite(BUZZER, LOW);  // 初始状态为低电平
}

void buzzer_beep(int frequency, int duration)
{
  // 使用tone函数产生PWM音调
  tone(BUZZER, frequency, duration);
  delay(duration);
  noTone(BUZZER);  // 停止音调
}

void startup_beep()
{
  // 开机提示音：1000Hz，持续200ms
  buzzer_beep(1000, 200);
  Serial.println("Startup beep completed");
}

void wifiScan(void);


void handle_at_wifiscan(const AT_Command *cmd)
{
  wifiScan();
}

void handle_at_version(const AT_Command *cmd)
{
  Serial.print("Firmware Version: ");
  Serial.println(FIRMWARE_VERSION);
  Serial.print("Build Date: ");
  Serial.println(BUILD_DATE);
  Serial.print("Build Time: ");
  Serial.println(BUILD_TIME);
  Serial.print("Hardware: RAK3112");
  Serial.println();
  Serial.print("RadioLib: SX1262 LoRa/FSK Module");
  Serial.println();
}

void handle_at_sd(const AT_Command *cmd)
{
  Serial.println("Starting SD card test...");
  test_sdcard();
}

void setupBoards(void)
{
  Serial.begin(115200);

  Serial.println("setupBoards");

  // RF Switch enable pin
  pinMode(4, OUTPUT);
  digitalWrite(4, HIGH);
  
  pinMode(14 , OUTPUT);
  digitalWrite(14 , HIGH);

  SPI.begin(RADIO_SCLK_PIN, RADIO_MISO_PIN, RADIO_MOSI_PIN);

  // Initialize SPI3
  // init_spi3();

  Serial.println("init done .");

  //Set WiFi to station mode and disconnect from an AP if it was previously connected.
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  register_at_handler("AT+WIFISCAN", handle_at_wifiscan, "Scan WiFi networks");
  register_at_handler("AT+VER", handle_at_version, "Query firmware version information");
  register_at_handler("AT+VERSION", handle_at_version, "Query firmware version information");
  register_at_handler("AT+SD", handle_at_sd, "Test SD card read/write operations");

  // esp_log_level_set("*", ESP_LOG_ERROR);          // 只显示错误级别
  esp_log_level_set("i2c.master", ESP_LOG_NONE);  // 完全关闭I2C日志

  // 按钮引脚初始化（0脚位，输入上拉）
  pinMode(0, INPUT_PULLUP);
}

void setup()
{
  setupBoards();
  init_buzzer();   // 初始化蜂鸣器
  init_lora_radio();
  init_command();
  init_ble();
  init_rak1904();  // Initialize RAK1904 accelerometer
  init_rak1921();
  init_gps();
  init_lcd();      // 初始化LCD显示屏 (配置SPI3总线)
  init_sdcard();   // 初始化SD卡 (重用LCD的SPI3总线)

  pinMode(BL, OUTPUT);
  digitalWrite(BL, HIGH); // Enable the backlight, you can also adjust the backlight brightness through PWM.
  
  // 开机响一声
  startup_beep();
}


void loop()
{
  receive_packet();
  fhss_auto_hop_send_loop();
  gpsParseDate();
  test_lcd_touch();
  check_button();
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


// 按钮检测函数，需在主循环调用
void check_button()
{
  static bool lastPressed = false;
  bool pressed = digitalRead(0) == LOW; // 按下为低电平
  if (pressed && !lastPressed) {
    startup_beep();
    Serial.println("BUTTON TEST OK");
  }
  lastPressed = pressed;
}