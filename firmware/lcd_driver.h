/**************************************************************************
 * @file lcd_driver.h
 * @brief TLSR8269 LCD显示屏驱动头文件
 * @author 全国首个第三方TLSR8269 C库
 * @note 支持I2C接口OLED/LCD，128x64分辨率
 **************************************************************************/

#ifndef __LCD_DRIVER_H
#define __LCD_DRIVER_H

#include <stdint.h>
#include <stdbool.h>
#include "tlsr8269_reg.h"

//==========================================================================
// 一、LCD配置
//==========================================================================
#define LCD_I2C_ADDR        0x3C    // I2C地址（SSD1306默认）
#define LCD_WIDTH           128     // 屏幕宽度
#define LCD_HEIGHT          64      // 屏幕高度
#define LCD_PAGES           8       // 页数（64/8）

// I2C引脚定义（使用PB0/PB3）
#define LCD_SCL_PIN         8       // PB0 - SCL
#define LCD_SDA_PIN         11      // PB3 - SDA
#define LCD_SCL_PORT        pb
#define LCD_SDA_PORT        pb

//==========================================================================
// 二、SSD1306命令定义
//==========================================================================
#define SSD1306_CMD         0x00    // 命令模式
#define SSD1306_DATA        0x40    // 数据模式

// 基础命令
#define SSD1306_DISPLAY_OFF     0xAE
#define SSD1306_DISPLAY_ON      0xAF
#define SSD1306_SET_CONTRAST    0x81
#define SSD1306_SET_COL_ADDR    0x21
#define SSD1306_SET_PAGE_ADDR   0x22
#define SSD1306_SET_START_LINE  0x40
#define SSD1306_SET_SEGMENT_REMAP   0xA1
#define SSD1306_SET_COM_SCAN_DIR    0xC8
#define SSD1306_SET_MUX_RATIO   0xA8
#define SSD1306_SET_DISPLAY_OFFSET  0xD3
#define SSD1306_SET_CLOCK_DIV   0xD5
#define SSD1306_SET_PRECHARGE   0xD9
#define SSD1306_SET_COM_PINS    0xDA
#define SSD1306_SET_VCOMH       0xDB
#define SSD1306_SET_CHARGE_PUMP 0x8D

//==========================================================================
// 三、数据类型
//==========================================================================
typedef enum {
    FONT_SMALL = 0,     // 6x8 字体
    FONT_MEDIUM,        // 8x16 字体
    FONT_LARGE          // 16x24 字体
} Font_SizeTypeDef;

typedef struct {
    uint8_t x;
    uint8_t y;
} LCD_PosTypeDef;

//==========================================================================
// 四、API函数声明
//==========================================================================

// 初始化
bool lcd_init(void);
void lcd_deinit(void);

// 基础控制
void lcd_clear(void);
void lcd_refresh(void);
void lcd_set_contrast(uint8_t contrast);
void lcd_display_on(void);
void lcd_display_off(void);

// 绘图
void lcd_draw_pixel(uint8_t x, uint8_t y, bool color);
void lcd_draw_line(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, bool color);
void lcd_draw_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, bool color);
void lcd_fill_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, bool color);
void lcd_draw_circle(uint8_t x, uint8_t y, uint8_t r, bool color);

// 文字显示
void lcd_set_font(Font_SizeTypeDef font);
void lcd_draw_char(uint8_t x, uint8_t y, char c, bool color);
void lcd_draw_string(uint8_t x, uint8_t y, const char *str, bool color);
void lcd_draw_number(uint8_t x, uint8_t y, int32_t num, bool color);
void lcd_draw_float(uint8_t x, uint8_t y, float num, uint8_t decimals, bool color);

// 界面元素
void lcd_draw_title(const char *title);
void lcd_draw_menu_item(uint8_t y, const char *text, bool selected);
void lcd_draw_progress_bar(uint8_t x, uint8_t y, uint8_t w, uint8_t percent);
void lcd_draw_channel_status(uint8_t ch, bool enabled, float freq, uint8_t intensity);

// 缓冲区操作
void lcd_set_buffer(uint8_t *buffer);
void lcd_get_buffer(uint8_t *buffer);

#endif /* __LCD_DRIVER_H */
