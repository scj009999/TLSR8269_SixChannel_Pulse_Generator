/**************************************************************************
 * @file button_driver.h
 * @brief TLSR8269 按键驱动头文件
 * @author 全国首个第三方TLSR8269 C库
 * @note 支持矩阵按键和独立按键，含消抖和长按检测
 **************************************************************************/

#ifndef __BUTTON_DRIVER_H
#define __BUTTON_DRIVER_H

#include <stdint.h>
#include <stdbool.h>
#include "tlsr8269_reg.h"

//==========================================================================
// 一、按键配置
//==========================================================================
#define BUTTON_MAX_NUM      8       // 最大按键数

// 按键引脚定义（使用PC0-PC5 + PD0-PD1）
#define BUTTON_UP_PIN       16      // PC0
#define BUTTON_DOWN_PIN     17      // PC1
#define BUTTON_LEFT_PIN     18      // PC2
#define BUTTON_RIGHT_PIN    19      // PC3
#define BUTTON_OK_PIN       20      // PC4
#define BUTTON_CANCEL_PIN   21      // PC5
#define BUTTON_START_PIN    24      // PD0
#define BUTTON_STOP_PIN     25      // PD1

#define BUTTON_PORT         pc      // 主要端口

//==========================================================================
// 二、按键事件类型
//==========================================================================
typedef enum {
    BUTTON_EVENT_NONE = 0,
    BUTTON_EVENT_PRESS,         // 按下
    BUTTON_EVENT_RELEASE,       // 释放
    BUTTON_EVENT_SHORT_PRESS,   // 短按（< 1秒）
    BUTTON_EVENT_LONG_PRESS,    // 长按（> 1秒）
    BUTTON_EVENT_DOUBLE_CLICK,  // 双击
    BUTTON_EVENT_HOLD           // 持续按住
} Button_EventTypeDef;

//==========================================================================
// 三、按键状态
//==========================================================================
typedef enum {
    BUTTON_STATE_IDLE = 0,
    BUTTON_STATE_DEBOUNCE,      // 消抖中
    BUTTON_STATE_PRESSED,       // 已确认按下
    BUTTON_STATE_LONG_PRESS,    // 长按确认
    BUTTON_STATE_RELEASED       // 已释放
} Button_StateTypeDef;

//==========================================================================
// 四、按键配置结构
//==========================================================================
typedef struct {
    uint8_t pin;                // 引脚号
    bool active_low;            // 低电平有效
    uint16_t debounce_ms;       // 消抖时间
    uint16_t long_press_ms;     // 长按时间
    uint16_t double_click_ms;   // 双击间隔
} Button_ConfigTypeDef;

// 按键实例结构
typedef struct {
    Button_ConfigTypeDef config;
    Button_StateTypeDef state;
    uint32_t press_time;        // 按下时间
    uint32_t release_time;      // 释放时间
    uint16_t hold_time;         // 按住时长
    bool event_pending;         // 有待处理事件
    Button_EventTypeDef last_event;
} Button_InstanceTypeDef;

//==========================================================================
// 五、API函数声明
//==========================================================================

// 初始化
bool button_init(void);
void button_deinit(void);

// 按键注册
bool button_register(uint8_t id, uint8_t pin, bool active_low);
bool button_register_config(uint8_t id, Button_ConfigTypeDef *config);

// 事件处理
Button_EventTypeDef button_get_event(uint8_t id);
bool button_is_pressed(uint8_t id);
bool button_is_long_pressed(uint8_t id);
uint16_t button_get_hold_time(uint8_t id);

// 扫描（需在主循环中定期调用）
void button_scan(void);

// 回调注册
typedef void (*Button_CallbackTypeDef)(uint8_t id, Button_EventTypeDef event);
void button_set_callback(Button_CallbackTypeDef callback);

// 便捷函数
uint8_t button_get_pressed_id(void);  // 获取当前按下的按键ID
void button_wait_for_release(uint8_t id);  // 等待释放

#endif /* __BUTTON_DRIVER_H */
