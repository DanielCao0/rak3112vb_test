#ifndef SDCARD_H
#define SDCARD_H

#include <SPI.h>
#include <SD.h>

// SD卡引脚定义 - 共享SPI3总线 (LCD已初始化)
#define SDCARD_CS      2    // SD卡片选引脚
// 注意：SCLK(13), MOSI(11), MISO(10) 由LCD模块管理

// 外部SPI3对象引用（在lcd.cpp中定义）
extern SPIClass spi3;

// SD卡初始化函数
bool init_sdcard();

// SD卡测试函数
void test_sdcard();

// SD卡写入测试文件
bool sdcard_write_test();

// SD卡读取测试文件
bool sdcard_read_test();

// SD卡信息显示
void sdcard_info();

// SD卡文件列表
void sdcard_list_files();

#endif // SDCARD_H
