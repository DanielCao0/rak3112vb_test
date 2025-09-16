#include "lcd.h"

// 推荐使用比例字体，提升清晰度
const lgfx::IFont *font_touch = &fonts::Font4; // Font4适合小屏幕，清晰大字

// 全局LCD对象实例
LGFX tft;

// LGFX构造函数实现
LGFX::LGFX(void)
{
  {
    auto cfg = _bus_instance.config();
    // SPI配置
    cfg.spi_host = SPI3_HOST;              // SPI外设，可以用 VSPI_HOST/HSPI_HOST/SPI3_HOST/FSPI 等
    cfg.spi_mode = 0;                      // SPI模式0（ST7789推荐）
    cfg.freq_write = 40000000;             // 写入频率，ST7789支持到40MHz
    cfg.freq_read = 16000000;              // 读频率
    cfg.pin_sclk = 13;                     // SPI SCLK（时钟）
    cfg.pin_mosi = 11;                     // SPI MOSI
    cfg.pin_miso = 10;                     // SPI MISO（如无可用，可设-1）
    cfg.pin_dc   = 42;                     // 数据/命令引脚
    // cfg.pin_cs   = 12;                     // 屏幕片选
    _bus_instance.config(cfg);
    _panel_instance.setBus(&_bus_instance);
  }

  {
    auto cfg = _panel_instance.config();
    cfg.pin_cs = 12;                       // 屏幕片选
    cfg.pin_rst = -1;                      // 复位
    cfg.pin_busy = -1;

    cfg.panel_width = screenWidth;
    cfg.panel_height = screenHeight;
    cfg.offset_x = 0;
    cfg.offset_y = 0;
    cfg.offset_rotation = offsetRotation;
    cfg.dummy_read_pixel = 8;
    cfg.dummy_read_bits = 1;
    cfg.readable = true;
    cfg.invert = true;
    cfg.rgb_order = false;
    cfg.dlen_16bit = false;
    cfg.bus_shared = true;

    _panel_instance.config(cfg);
  }

  {
    auto cfg = _touch_instance.config();

    cfg.x_min = 0;
    cfg.x_max = screenWidth - 1;
    cfg.y_min = 0;
    cfg.y_max = screenHeight - 1;
    cfg.pin_int = 39;
    cfg.bus_shared = true;
    cfg.offset_rotation = 2;
    // I2C
    cfg.i2c_port = 0;
    cfg.i2c_addr = 0x38;
    cfg.pin_sda = 9;
    cfg.pin_scl = 40;
    cfg.freq = 400000;

    _touch_instance.config(cfg);
    _panel_instance.setTouch(&_touch_instance);
  }
  setPanel(&_panel_instance);
}

// LCD初始化函数
void init_lcd()
{
  // 电源控制引脚 
  pinMode(14, OUTPUT);
  digitalWrite(14, HIGH);  // 打开电源

  // LED控制引脚
  pinMode(45, OUTPUT);
  digitalWrite(45, HIGH);  // 打开LED

  // 背光控制引脚
  pinMode(41, OUTPUT);
  digitalWrite(41, HIGH);  // 打开背光

  // 初始化LCD
  tft.init();
  
  // 设置默认显示
  tft.fillScreen(TFT_BLUE);
  tft.setFont(font_touch);
  tft.setTextColor(TFT_WHITE, TFT_BLUE);
  
  // 显示初始化信息
  String msg = "RAK3112 LCD Ready";
  int16_t msg_x = (tft.width() - tft.textWidth(msg.c_str())) / 2;
  int16_t msg_y = (tft.height() - tft.fontHeight()) / 2;
  
  tft.setCursor(msg_x, msg_y);
  tft.print(msg);
  
  Serial.println("LCD initialized successfully on SPI3");
}

// LCD显示文本函数
void lcd_display_text(const char* text, uint16_t bg_color, uint16_t text_color)
{
  tft.fillScreen(bg_color);
  tft.setFont(font_touch);
  tft.setTextColor(text_color, bg_color);
  
  int16_t msg_x = (tft.width() - tft.textWidth(text)) / 2;
  int16_t msg_y = (tft.height() - tft.fontHeight()) / 2;
  
  tft.setCursor(msg_x, msg_y);
  tft.print(text);
}

// LCD触摸测试函数
void test_lcd_touch()
{
  static int16_t last_x = -32768, last_y = -32768;
  static bool last_touched = false;  // 记录上次的触摸状态
  int16_t x = -1, y = -1;

  // 颜色数组，每次变化切换一种
  const uint16_t colors[] = {TFT_RED, TFT_GREEN, TFT_BLUE, TFT_CYAN, TFT_MAGENTA, TFT_YELLOW, TFT_BLACK, TFT_WHITE};
  const int colorCount = sizeof(colors) / sizeof(colors[0]);
  static int colorIndex = 0;

  // 获取触摸点
  bool touched = tft.getTouch(&x, &y);

  // 检测到新的触摸事件（从未触摸到触摸）
  if (touched && !last_touched) {
    // 触摸响声：800Hz，持续50ms
    buzzer_beep(800, 50);
  }

  // 只在坐标有变化时刷新
  if (x != last_x || y != last_y)
  {
    // 切换背景色
    colorIndex = (colorIndex + 1) % colorCount;
    uint16_t bgColor = colors[colorIndex];

    // 居中显示坐标
    String msg;
    if (touched)
    {
      msg = String("Touch: ") + x + ", " + y;
    //   Serial.printf("Touch: %3d, %3d\r\n", x, y);
    }
    else
    {
      msg = "Touch: None";
    //   Serial.println("Touch: None");
    }

    // 整屏变色
    tft.fillScreen(bgColor);

    // 覆盖旧内容（这里是新的色块）
    int16_t msg_x = (tft.width() - tft.textWidth(msg.c_str())) / 2;
    int16_t msg_y = (tft.height() - tft.fontHeight()) / 2;

    // 字体设置，颜色建议与背景色反差大
    tft.setFont(font_touch);
    tft.setTextColor(TFT_BLACK, bgColor); // 如果背景色亮，字体黑色，否则你可以改TFT_WHITE

    tft.setCursor(msg_x, msg_y);
    tft.print(msg);

    last_x = x;
    last_y = y;
  }
  
  // 更新上次触摸状态
  last_touched = touched;
}
