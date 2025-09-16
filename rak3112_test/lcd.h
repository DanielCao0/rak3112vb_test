#ifndef LCD_H
#define LCD_H

#include <LovyanGFX.hpp>
#include <Wire.h>
#include <SPI.h>

// 蜂鸣器引脚定义
#define BUZZER        38

// SPI3引脚定义
#define SPI3_SCLK     13
#define SPI3_MISO     10  
#define SPI3_MOSI     11

// 蜂鸣器函数声明（在主程序中实现）
extern void buzzer_beep(int frequency, int duration);

// 全局SPI3对象声明（在lcd.cpp中定义）
extern SPIClass spi3;

// LCD显示屏控制类
class LGFX : public lgfx::LGFX_Device
{
  lgfx::Panel_ST7789 _panel_instance;
  lgfx::Bus_SPI _bus_instance;               // 用SPI总线
  lgfx::Touch_FT5x06 _touch_instance;

  const uint16_t screenWidth = 240;
  const uint16_t screenHeight = 320;
  const uint8_t offsetRotation = 0;

public:
  LGFX(void);
};

// 全局LCD对象
extern LGFX tft;

// LCD初始化函数
void init_lcd();

// LCD测试函数（显示触摸坐标）
void test_lcd_touch();

// LCD显示文本函数
void lcd_display_text(const char* text, uint16_t bg_color = TFT_BLUE, uint16_t text_color = TFT_WHITE);

#endif // LCD_H
