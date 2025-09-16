#include "sdcard.h"

// SD卡初始化状态
static bool sdcard_initialized = false;
SPIClass SPI_HSPI(HSPI);
#define SDHandler SPI_HSPI


// 注意：spi3对象在lcd.cpp中定义和初始化
// extern SPIClass spi3; 已在sdcard.h中声明

// SD卡初始化函数
bool init_sdcard()
{
  Serial.println("Initializing SD card on shared SPI3 (after LCD)...");
  
  // LCD已经初始化了SPI3总线，这里直接使用
  // 设置CS引脚为输出
  pinMode(SDCARD_CS, OUTPUT);
  digitalWrite(SDCARD_CS, HIGH);  // 默认拉高，释放总线
  
  // 短暂延迟，确保LCD初始化完成
  delay(100);

  SDHandler.begin(13, 10, 11);
  
  // 使用LCD已初始化的SPI3总线初始化SD卡
  // 明确指定使用spi3对象（由LCD模块创建和初始化）
  if (!SD.begin(SDCARD_CS, SDHandler)) {
    Serial.println("SD card initialization failed!");
    Serial.println("Check:");
    Serial.println("* SD card is inserted");
    Serial.println("* SD card is formatted (FAT16/FAT32)");
    Serial.println("* Shared SPI3 wiring is correct:");
    Serial.printf("  - CS:   Pin %d (SD card)\n", SDCARD_CS);
    Serial.println("  - SCLK: Pin 13 (shared SPI3 with LCD)");
    Serial.println("  - MOSI: Pin 11 (shared SPI3 with LCD)");
    Serial.println("  - MISO: Pin 10 (shared SPI3 with LCD)");
    Serial.println("* LCD initialization completed first (creates spi3 object)");
    Serial.println("* Using explicit spi3 object for SD.begin()");
    sdcard_initialized = false;
    return false;
  }
  
  Serial.println("SD card initialization successful on shared SPI3!");
  Serial.println("Using explicit spi3 object initialized by LCD module");
  sdcard_initialized = true;
  
  // 显示SD卡信息
  sdcard_info();
  
  return true;
}

// SD卡信息显示
void sdcard_info()
{
  if (!sdcard_initialized) {
    Serial.println("SD card not initialized!");
    return;
  }
  
  Serial.println("\n=== SD Card Information ===");
  
  // 获取SD卡类型
  uint8_t cardType = SD.cardType();
  Serial.print("Card Type: ");
  if (cardType == CARD_NONE) {
    Serial.println("No SD card attached");
    return;
  } else if (cardType == CARD_MMC) {
    Serial.println("MMC");
  } else if (cardType == CARD_SD) {
    Serial.println("SDSC");
  } else if (cardType == CARD_SDHC) {
    Serial.println("SDHC");
  } else {
    Serial.println("UNKNOWN");
  }
  
  // 获取SD卡大小
  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  Serial.printf("Card Size: %lluMB\n", cardSize);
  
  // 获取已用空间
  uint64_t totalBytes = SD.totalBytes() / (1024 * 1024);
  uint64_t usedBytes = SD.usedBytes() / (1024 * 1024);
  Serial.printf("Total space: %lluMB\n", totalBytes);
  Serial.printf("Used space: %lluMB\n", usedBytes);
  Serial.printf("Free space: %lluMB\n", totalBytes - usedBytes);
  
  Serial.println("===========================\n");
}

// SD卡文件列表
void sdcard_list_files()
{
  if (!sdcard_initialized) {
    Serial.println("SD card not initialized!");
    return;
  }
  
  Serial.println("\n=== SD Card File List ===");
  File root = SD.open("/");
  if (!root) {
    Serial.println("Failed to open directory");
    return;
  }
  
  if (!root.isDirectory()) {
    Serial.println("Not a directory");
    return;
  }
  
  File file = root.openNextFile();
  int fileCount = 0;
  while (file) {
    if (file.isDirectory()) {
      Serial.print("DIR:  ");
      Serial.println(file.name());
    } else {
      Serial.print("FILE: ");
      Serial.print(file.name());
      Serial.print("\t\t");
      Serial.print(file.size());
      Serial.println(" bytes");
    }
    file = root.openNextFile();
    fileCount++;
  }
  
  if (fileCount == 0) {
    Serial.println("No files found");
  } else {
    Serial.printf("Total: %d items\n", fileCount);
  }
  
  Serial.println("========================\n");
}

// SD卡写入测试文件
bool sdcard_write_test()
{
  if (!sdcard_initialized) {
    Serial.println("SD card not initialized!");
    return false;
  }
  
  Serial.println("Testing SD card write...");
  
  // 创建测试文件
  File testFile = SD.open("/test.txt", FILE_WRITE);
  if (!testFile) {
    Serial.println("Failed to create test file!");
    return false;
  }
  
  // 写入测试数据
  String testData = "RAK3112 SD Card Test\n";
  testData += "Timestamp: ";
  testData += millis();
  testData += " ms\n";
  testData += "This is a test file for SD card functionality.\n";
  testData += "Line 1: Hello World!\n";
  testData += "Line 2: ESP32-S3 with SD card\n";
  testData += "Line 3: SPI communication test\n";
  testData += "End of test data.\n";
  
  size_t bytesWritten = testFile.print(testData);
  testFile.close();
  
  if (bytesWritten > 0) {
    Serial.printf("Write test successful! Wrote %d bytes to /test.txt\n", bytesWritten);
    return true;
  } else {
    Serial.println("Write test failed!");
    return false;
  }
}

// SD卡读取测试文件
bool sdcard_read_test()
{
  if (!sdcard_initialized) {
    Serial.println("SD card not initialized!");
    return false;
  }
  
  Serial.println("Testing SD card read...");
  
  // 打开测试文件
  File testFile = SD.open("/test.txt");
  if (!testFile) {
    Serial.println("Failed to open test file!");
    Serial.println("Please run write test first (the test file doesn't exist)");
    return false;
  }
  
  Serial.println("=== File Content ===");
  
  // 读取并显示文件内容
  size_t bytesRead = 0;
  while (testFile.available()) {
    char c = testFile.read();
    Serial.print(c);
    bytesRead++;
  }
  testFile.close();
  
  Serial.println("==================");
  Serial.printf("Read test successful! Read %d bytes from /test.txt\n", bytesRead);
  
  return true;
}

// SD卡简单测试函数
void test_sdcard()
{
  // 如果还未初始化，先尝试初始化
  if (!sdcard_initialized) {
    if (!init_sdcard()) {
      Serial.println("SD TEST FAIL");
      return;
    }
  }
  
  // 简单检测SD卡是否可用
  if (SD.cardType() != CARD_NONE) {
    Serial.println("SD TEST OK");
  } else {
    Serial.println("SD TEST FAIL");
  }
}
