/**************************************************************************
 * @file lcd_driver.c
 * @brief TLSR8269 LCD显示屏驱动实现（SSD1306 I2C）
 * @author 全国首个第三方TLSR8269 C库
 * @note 支持128x64 OLED，I2C接口
 **************************************************************************/

#include "lcd_driver.h"
#include "i2c_driver.h"
#include <string.h>

//==========================================================================
// 一、字库数据
//==========================================================================

// 6x8 ASCII字库（96个字符，从空格0x20开始）
static const uint8_t font_6x8[][6] = {
    {0x00,0x00,0x00,0x00,0x00,0x00}, // 空格
    {0x00,0x00,0x5F,0x00,0x00,0x00}, // !
    {0x00,0x07,0x00,0x07,0x00,0x00}, // "
    // ... 更多字符
    {0x3E,0x51,0x49,0x45,0x3E,0x00}, // 0
    {0x00,0x42,0x7F,0x40,0x00,0x00}, // 1
    {0x42,0x61,0x51,0x49,0x46,0x00}, // 2
    {0x21,0x41,0x45,0x4B,0x31,0x00}, // 3
    {0x18,0x14,0x12,0x7F,0x10,0x00}, // 4
    {0x27,0x45,0x45,0x45,0x39,0x00}, // 5
    {0x3C,0x4A,0x49,0x49,0x30,0x00}, // 6
    {0x01,0x71,0x09,0x05,0x03,0x00}, // 7
    {0x36,0x49,0x49,0x49,0x36,0x00}, // 8
    {0x06,0x49,0x49,0x29,0x1E,0x00}, // 9
    // ... A-Z, a-z等
};

// 8x16 ASCII字库
static const uint8_t font_8x16[][16] = {
    // 数字0-9
    {0x00,0x00,0x3E,0x7F,0x51,0x49,0x45,0x7F,0x3E,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, // 0
    {0x00,0x00,0x00,0x42,0x7F,0x7F,0x40,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, // 1
    // ... 更多字符
};

//==========================================================================
// 二、全局变量
//==========================================================================
static uint8_t lcd_buffer[LCD_WIDTH * LCD_PAGES];  // 显存
static Font_SizeTypeDef current_font = FONT_SMALL;
static bool lcd_initialized = false;

//==========================================================================
// 三、I2C底层操作
//==========================================================================
static bool lcd_write_cmd(uint8_t cmd)
{
    uint8_t data[2] = {SSD1306_CMD, cmd};
    return i2c_write(LCD_I2C_ADDR, data, 2);
}

static bool lcd_write_data(uint8_t *data, uint16_t len)
{
    uint8_t tx_buf[17];
    tx_buf[0] = SSD1306_DATA;
    
    // 分块发送（每块16字节）
    uint16_t sent = 0;
    while (sent < len) {
        uint8_t chunk = (len - sent > 16) ? 16 : (len - sent);
        memcpy(&tx_buf[1], &data[sent], chunk);
        if (!i2c_write(LCD_I2C_ADDR, tx_buf, chunk + 1)) {
            return false;
        }
        sent += chunk;
    }
    return true;
}

//==========================================================================
// 四、初始化
//==========================================================================
bool lcd_init(void)
{
    if (lcd_initialized) {
        return true;
    }
    
    // 初始化I2C
    if (!i2c_init(LCD_SCL_PIN, LCD_SDA_PIN, 400000)) {  // 400kHz
        return false;
    }
    
    // 等待设备上电
    delay_ms(100);
    
    // SSD1306初始化序列
    lcd_write_cmd(SSD1306_DISPLAY_OFF);
    lcd_write_cmd(SSD1306_SET_CLOCK_DIV);
    lcd_write_cmd(0x80);
    lcd_write_cmd(SSD1306_SET_MUX_RATIO);
    lcd_write_cmd(0x3F);  // 64行
    lcd_write_cmd(SSD1306_SET_DISPLAY_OFFSET);
    lcd_write_cmd(0x00);
    lcd_write_cmd(SSD1306_SET_START_LINE | 0x00);
    lcd_write_cmd(SSD1306_SET_CHARGE_PUMP);
    lcd_write_cmd(0x14);  // 使能电荷泵
    lcd_write_cmd(SSD1306_SET_SEGMENT_REMAP);
    lcd_write_cmd(SSD1306_SET_COM_SCAN_DIR);
    lcd_write_cmd(SSD1306_SET_COM_PINS);
    lcd_write_cmd(0x12);  // 替代COM引脚配置
    lcd_write_cmd(SSD1306_SET_CONTRAST);
    lcd_write_cmd(0xCF);  // 对比度
    lcd_write_cmd(SSD1306_SET_PRECHARGE);
    lcd_write_cmd(0xF1);
    lcd_write_cmd(SSD1306_SET_VCOMH);
    lcd_write_cmd(0x40);
    lcd_write_cmd(SSD1306_DISPLAY_ON);
    
    // 清屏
    lcd_clear();
    lcd_refresh();
    
    lcd_initialized = true;
    return true;
}

void lcd_deinit(void)
{
    lcd_write_cmd(SSD1306_DISPLAY_OFF);
    i2c_deinit();
    lcd_initialized = false;
}

//==========================================================================
// 五、基础控制
//==========================================================================
void lcd_clear(void)
{
    memset(lcd_buffer, 0x00, sizeof(lcd_buffer));
}

void lcd_refresh(void)
{
    // 设置列地址
    lcd_write_cmd(SSD1306_SET_COL_ADDR);
    lcd_write_cmd(0x00);
    lcd_write_cmd(LCD_WIDTH - 1);
    
    // 设置页地址
    lcd_write_cmd(SSD1306_SET_PAGE_ADDR);
    lcd_write_cmd(0x00);
    lcd_write_cmd(LCD_PAGES - 1);
    
    // 发送显存数据
    lcd_write_data(lcd_buffer, sizeof(lcd_buffer));
}

void lcd_set_contrast(uint8_t contrast)
{
    lcd_write_cmd(SSD1306_SET_CONTRAST);
    lcd_write_cmd(contrast);
}

void lcd_display_on(void)
{
    lcd_write_cmd(SSD1306_DISPLAY_ON);
}

void lcd_display_off(void)
{
    lcd_write_cmd(SSD1306_DISPLAY_OFF);
}

//==========================================================================
// 六、绘图函数
//==========================================================================
void lcd_draw_pixel(uint8_t x, uint8_t y, bool color)
{
    if (x >= LCD_WIDTH || y >= LCD_HEIGHT) {
        return;
    }
    
    uint16_t index = x + (y / 8) * LCD_WIDTH;
    uint8_t bit = y % 8;
    
    if (color) {
        lcd_buffer[index] |= (1 << bit);
    } else {
        lcd_buffer[index] &= ~(1 << bit);
    }
}

void lcd_draw_line(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, bool color)
{
    int16_t dx = abs(x1 - x0);
    int16_t dy = abs(y1 - y0);
    int16_t sx = (x0 < x1) ? 1 : -1;
    int16_t sy = (y0 < y1) ? 1 : -1;
    int16_t err = dx - dy;
    
    while (1) {
        lcd_draw_pixel(x0, y0, color);
        
        if (x0 == x1 && y0 == y1) break;
        
        int16_t e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x0 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y0 += sy;
        }
    }
}

void lcd_draw_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, bool color)
{
    lcd_draw_line(x, y, x + w - 1, y, color);
    lcd_draw_line(x + w - 1, y, x + w - 1, y + h - 1, color);
    lcd_draw_line(x + w - 1, y + h - 1, x, y + h - 1, color);
    lcd_draw_line(x, y + h - 1, x, y, color);
}

void lcd_fill_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, bool color)
{
    for (uint8_t i = x; i < x + w; i++) {
        for (uint8_t j = y; j < y + h; j++) {
            lcd_draw_pixel(i, j, color);
        }
    }
}

void lcd_draw_circle(uint8_t x, uint8_t y, uint8_t r, bool color)
{
    int16_t f = 1 - r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * r;
    int16_t x0 = 0;
    int16_t y0 = r;
    
    lcd_draw_pixel(x, y + r, color);
    lcd_draw_pixel(x, y - r, color);
    lcd_draw_pixel(x + r, y, color);
    lcd_draw_pixel(x - r, y, color);
    
    while (x0 < y0) {
        if (f >= 0) {
            y0--;
            ddF_y += 2;
            f += ddF_y;
        }
        x0++;
        ddF_x += 2;
        f += ddF_x;
        
        lcd_draw_pixel(x + x0, y + y0, color);
        lcd_draw_pixel(x - x0, y + y0, color);
        lcd_draw_pixel(x + x0, y - y0, color);
        lcd_draw_pixel(x - x0, y - y0, color);
        lcd_draw_pixel(x + y0, y + x0, color);
        lcd_draw_pixel(x - y0, y + x0, color);
        lcd_draw_pixel(x + y0, y - x0, color);
        lcd_draw_pixel(x - y0, y - x0, color);
    }
}

//==========================================================================
// 七、文字显示
//==========================================================================
void lcd_set_font(Font_SizeTypeDef font)
{
    current_font = font;
}

void lcd_draw_char(uint8_t x, uint8_t y, char c, bool color)
{
    if (c < 32 || c > 127) return;
    
    uint8_t index = c - 32;
    
    switch (current_font) {
        case FONT_SMALL:
            if (index < sizeof(font_6x8) / 6) {
                for (uint8_t i = 0; i < 6; i++) {
                    uint8_t line = font_6x8[index][i];
                    for (uint8_t j = 0; j < 8; j++) {
                        if (line & (1 << j)) {
                            lcd_draw_pixel(x + i, y + j, color);
                        }
                    }
                }
            }
            break;
            
        case FONT_MEDIUM:
            // 8x16字体
            if (index < sizeof(font_8x16) / 16) {
                for (uint8_t i = 0; i < 8; i++) {
                    uint8_t line = font_8x16[index][i];
                    for (uint8_t j = 0; j < 8; j++) {
                        if (line & (1 << j)) {
                            lcd_draw_pixel(x + i, y + j, color);
                        }
                    }
                }
                for (uint8_t i = 0; i < 8; i++) {
                    uint8_t line = font_8x16[index][i + 8];
                    for (uint8_t j = 0; j < 8; j++) {
                        if (line & (1 << j)) {
                            lcd_draw_pixel(x + i, y + 8 + j, color);
                        }
                    }
                }
            }
            break;
            
        default:
            break;
    }
}

void lcd_draw_string(uint8_t x, uint8_t y, const char *str, bool color)
{
    uint8_t x_pos = x;
    uint8_t char_width = (current_font == FONT_SMALL) ? 6 : 8;
    
    while (*str) {
        if (*str == '\n') {
            x_pos = x;
            y += (current_font == FONT_SMALL) ? 8 : 16;
        } else {
            lcd_draw_char(x_pos, y, *str, color);
            x_pos += char_width;
        }
        str++;
    }
}

void lcd_draw_number(uint8_t x, uint8_t y, int32_t num, bool color)
{
    char buf[12];
    char *p = buf + 11;
    *p = '\0';
    
    bool negative = (num < 0);
    if (negative) num = -num;
    
    do {
        *--p = '0' + (num % 10);
        num /= 10;
    } while (num > 0);
    
    if (negative) *--p = '-';
    
    lcd_draw_string(x, y, p, color);
}

void lcd_draw_float(uint8_t x, uint8_t y, float num, uint8_t decimals, bool color)
{
    char buf[16];
    int32_t integer = (int32_t)num;
    float fractional = num - integer;
    
    if (num < 0) {
        fractional = -fractional;
    }
    
    // 整数部分
    lcd_draw_number(x, y, integer, color);
    
    // 小数点
    uint8_t x_offset = x + 6 * 3;  // 假设3位整数
    lcd_draw_char(x_offset, y, '.', color);
    
    // 小数部分
    for (uint8_t i = 0; i < decimals; i++) {
        fractional *= 10;
        int32_t digit = (int32_t)fractional;
        lcd_draw_char(x_offset + 6 * (i + 1), y, '0' + digit, color);
        fractional -= digit;
    }
}

//==========================================================================
// 八、界面元素
//==========================================================================
void lcd_draw_title(const char *title)
{
    // 绘制标题栏背景
    lcd_fill_rect(0, 0, LCD_WIDTH, 12, true);
    
    // 绘制标题文字（反色）
    uint8_t len = strlen(title);
    uint8_t x = (LCD_WIDTH - len * 6) / 2;
    lcd_draw_string(x, 2, title, false);
}

void lcd_draw_menu_item(uint8_t y, const char *text, bool selected)
{
    if (selected) {
        lcd_fill_rect(0, y, LCD_WIDTH, 10, true);
        lcd_draw_string(2, y + 1, text, false);
    } else {
        lcd_fill_rect(0, y, LCD_WIDTH, 10, false);
        lcd_draw_string(2, y + 1, text, true);
    }
}

void lcd_draw_progress_bar(uint8_t x, uint8_t y, uint8_t w, uint8_t percent)
{
    uint8_t filled = (w * percent) / 100;
    
    // 边框
    lcd_draw_rect(x, y, w, 6, true);
    
    // 填充
    if (filled > 0) {
        lcd_fill_rect(x + 1, y + 1, filled - 2, 4, true);
    }
}

void lcd_draw_channel_status(uint8_t ch, bool enabled, float freq, uint8_t intensity)
{
    uint8_t y = 16 + ch * 8;  // 从第16行开始，每通道8像素
    char buf[32];
    
    // 通道号
    lcd_draw_char(0, y, '0' + ch, true);
    lcd_draw_char(6, y, ':', true);
    
    if (enabled) {
        // 状态：ON
        lcd_draw_string(12, y, "ON ", true);
        
        // 频率
        lcd_draw_float(30, y, freq, 1, true);
        lcd_draw_string(66, y, "Hz", true);
        
        // 强度条
        lcd_draw_progress_bar(90, y + 1, 30, intensity);
    } else {
        // 状态：OFF
        lcd_draw_string(12, y, "OFF", true);
    }
}

//==========================================================================
// 九、缓冲区操作
//==========================================================================
void lcd_set_buffer(uint8_t *buffer)
{
    memcpy(lcd_buffer, buffer, sizeof(lcd_buffer));
}

void lcd_get_buffer(uint8_t *buffer)
{
    memcpy(buffer, lcd_buffer, sizeof(lcd_buffer));
}
