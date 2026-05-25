/**************************************************************************
 * @file button_driver.c
 * @brief TLSR8269 按键驱动实现
 * @author 全国首个第三方TLSR8269 C库
 * @note 支持消抖、长按、双击检测
 **************************************************************************/

#include "button_driver.h"
#include "timer_driver.h"

//==========================================================================
// 一、默认配置
//==========================================================================
static const Button_ConfigTypeDef button_default_config = {
    .active_low = true,         // 默认低电平有效（上拉输入）
    .debounce_ms = 20,          // 20ms消抖
    .long_press_ms = 1000,      // 1秒长按
    .double_click_ms = 300      // 300ms双击间隔
};

//==========================================================================
// 二、全局变量
//==========================================================================
static Button_InstanceTypeDef buttons[BUTTON_MAX_NUM];
static Button_CallbackTypeDef button_callback = NULL;
static bool button_initialized = false;

//==========================================================================
// 三、GPIO操作
//==========================================================================
static bool button_read_pin(uint8_t pin)
{
    // 根据引脚号选择端口
    if (pin < 16) {
        // PA端口
        return GPIO_READ(pa, pin);
    } else if (pin < 24) {
        // PB端口
        return GPIO_READ(pb, pin - 16);
    } else if (pin < 32) {
        // PC端口
        return GPIO_READ(pc, pin - 24);
    }
    return false;
}

static void button_init_pin(uint8_t pin)
{
    // 配置为上拉输入
    if (pin < 16) {
        GPIO_SET_INPUT(pa, pin);
        GPIO_SET_PULLUP(pa, pin);
    } else if (pin < 24) {
        GPIO_SET_INPUT(pb, pin - 16);
        GPIO_SET_PULLUP(pb, pin - 16);
    } else if (pin < 32) {
        GPIO_SET_INPUT(pc, pin - 24);
        GPIO_SET_PULLUP(pc, pin - 24);
    }
}

//==========================================================================
// 四、初始化
//==========================================================================
bool button_init(void)
{
    if (button_initialized) {
        return true;
    }
    
    // 初始化所有按键实例
    for (uint8_t i = 0; i < BUTTON_MAX_NUM; i++) {
        memset(&buttons[i], 0, sizeof(Button_InstanceTypeDef));
        buttons[i].state = BUTTON_STATE_IDLE;
        buttons[i].config = button_default_config;
    }
    
    button_initialized = true;
    return true;
}

void button_deinit(void)
{
    button_initialized = false;
    button_callback = NULL;
}

//==========================================================================
// 五、按键注册
//==========================================================================
bool button_register(uint8_t id, uint8_t pin, bool active_low)
{
    if (id >= BUTTON_MAX_NUM || !button_initialized) {
        return false;
    }
    
    buttons[id].config.pin = pin;
    buttons[id].config.active_low = active_low;
    buttons[id].config.debounce_ms = button_default_config.debounce_ms;
    buttons[id].config.long_press_ms = button_default_config.long_press_ms;
    buttons[id].config.double_click_ms = button_default_config.double_click_ms;
    
    // 初始化GPIO
    button_init_pin(pin);
    
    return true;
}

bool button_register_config(uint8_t id, Button_ConfigTypeDef *config)
{
    if (id >= BUTTON_MAX_NUM || config == NULL || !button_initialized) {
        return false;
    }
    
    memcpy(&buttons[id].config, config, sizeof(Button_ConfigTypeDef));
    button_init_pin(config->pin);
    
    return true;
}

//==========================================================================
// 六、事件处理
//==========================================================================
Button_EventTypeDef button_get_event(uint8_t id)
{
    if (id >= BUTTON_MAX_NUM) {
        return BUTTON_EVENT_NONE;
    }
    
    Button_EventTypeDef event = buttons[id].last_event;
    buttons[id].last_event = BUTTON_EVENT_NONE;
    buttons[id].event_pending = false;
    
    return event;
}

bool button_is_pressed(uint8_t id)
{
    if (id >= BUTTON_MAX_NUM) {
        return false;
    }
    
    return (buttons[id].state == BUTTON_STATE_PRESSED || 
            buttons[id].state == BUTTON_STATE_LONG_PRESS);
}

bool button_is_long_pressed(uint8_t id)
{
    if (id >= BUTTON_MAX_NUM) {
        return false;
    }
    
    return (buttons[id].state == BUTTON_STATE_LONG_PRESS);
}

uint16_t button_get_hold_time(uint8_t id)
{
    if (id >= BUTTON_MAX_NUM) {
        return 0;
    }
    
    return buttons[id].hold_time;
}

uint8_t button_get_pressed_id(void)
{
    for (uint8_t i = 0; i < BUTTON_MAX_NUM; i++) {
        if (button_is_pressed(i)) {
            return i;
        }
    }
    return 0xFF;  // 无按键按下
}

void button_wait_for_release(uint8_t id)
{
    if (id >= BUTTON_MAX_NUM) {
        return;
    }
    
    while (button_is_pressed(id)) {
        button_scan();
        delay_ms(10);
    }
}

//==========================================================================
// 七、按键扫描（核心算法）
//==========================================================================
void button_scan(void)
{
    if (!button_initialized) {
        return;
    }
    
    static uint32_t last_scan_time = 0;
    uint32_t current_time = get_system_tick();
    
    // 限制扫描频率（每10ms扫描一次）
    if (current_time - last_scan_time < 10) {
        return;
    }
    last_scan_time = current_time;
    
    for (uint8_t i = 0; i < BUTTON_MAX_NUM; i++) {
        Button_InstanceTypeDef *btn = &buttons[i];
        
        // 跳过未注册的按键
        if (btn->config.pin == 0 && i > 0) {
            continue;
        }
        
        // 读取按键状态
        bool pin_state = button_read_pin(btn->config.pin);
        bool pressed = (btn->config.active_low) ? !pin_state : pin_state;
        
        switch (btn->state) {
            case BUTTON_STATE_IDLE:
                if (pressed) {
                    btn->press_time = current_time;
                    btn->state = BUTTON_STATE_DEBOUNCE;
                }
                break;
                
            case BUTTON_STATE_DEBOUNCE:
                if (pressed) {
                    // 确认按下
                    if (current_time - btn->press_time >= btn->config.debounce_ms) {
                        btn->state = BUTTON_STATE_PRESSED;
                        btn->hold_time = 0;
                        btn->last_event = BUTTON_EVENT_PRESS;
                        btn->event_pending = true;
                        
                        // 触发回调
                        if (button_callback) {
                            button_callback(i, BUTTON_EVENT_PRESS);
                        }
                    }
                } else {
                    // 消抖失败，回到空闲
                    btn->state = BUTTON_STATE_IDLE;
                }
                break;
                
            case BUTTON_STATE_PRESSED:
                if (pressed) {
                    btn->hold_time = current_time - btn->press_time;
                    
                    // 检测长按
                    if (btn->hold_time >= btn->config.long_press_ms) {
                        btn->state = BUTTON_STATE_LONG_PRESS;
                        btn->last_event = BUTTON_EVENT_LONG_PRESS;
                        btn->event_pending = true;
                        
                        if (button_callback) {
                            button_callback(i, BUTTON_EVENT_LONG_PRESS);
                        }
                    }
                } else {
                    // 释放
                    btn->release_time = current_time;
                    btn->hold_time = current_time - btn->press_time;
                    btn->state = BUTTON_STATE_RELEASED;
                    
                    // 判断短按还是双击
                    if (btn->hold_time < btn->config.long_press_ms) {
                        btn->last_event = BUTTON_EVENT_SHORT_PRESS;
                        btn->event_pending = true;
                        
                        if (button_callback) {
                            button_callback(i, BUTTON_EVENT_SHORT_PRESS);
                        }
                    }
                }
                break;
                
            case BUTTON_STATE_LONG_PRESS:
                if (pressed) {
                    btn->hold_time = current_time - btn->press_time;
                    
                    // 持续按住事件
                    if (button_callback && (btn->hold_time % 500) < 10) {
                        button_callback(i, BUTTON_EVENT_HOLD);
                    }
                } else {
                    // 长按释放
                    btn->release_time = current_time;
                    btn->state = BUTTON_STATE_RELEASED;
                    btn->last_event = BUTTON_EVENT_RELEASE;
                    btn->event_pending = true;
                    
                    if (button_callback) {
                        button_callback(i, BUTTON_EVENT_RELEASE);
                    }
                }
                break;
                
            case BUTTON_STATE_RELEASED:
                // 等待一段时间后再回到空闲，用于检测双击
                if (current_time - btn->release_time >= btn->config.double_click_ms) {
                    btn->state = BUTTON_STATE_IDLE;
                } else if (pressed) {
                    // 检测到双击
                    btn->state = BUTTON_STATE_DEBOUNCE;
                    btn->press_time = current_time;
                    btn->last_event = BUTTON_EVENT_DOUBLE_CLICK;
                    btn->event_pending = true;
                    
                    if (button_callback) {
                        button_callback(i, BUTTON_EVENT_DOUBLE_CLICK);
                    }
                }
                break;
                
            default:
                btn->state = BUTTON_STATE_IDLE;
                break;
        }
    }
}

//==========================================================================
// 八、回调注册
//==========================================================================
void button_set_callback(Button_CallbackTypeDef callback)
{
    button_callback = callback;
}

//==========================================================================
// 九、便捷函数：按键映射表
//==========================================================================
// 预定义的按键ID
#define BTN_UP      0
#define BTN_DOWN    1
#define BTN_LEFT    2
#define BTN_RIGHT   3
#define BTN_OK      4
#define BTN_CANCEL  5
#define BTN_START   6
#define BTN_STOP    7

bool button_init_default_layout(void)
{
    if (!button_init()) {
        return false;
    }
    
    // 注册默认按键布局
    button_register(BTN_UP,      BUTTON_UP_PIN,      true);
    button_register(BTN_DOWN,    BUTTON_DOWN_PIN,    true);
    button_register(BTN_LEFT,    BUTTON_LEFT_PIN,    true);
    button_register(BTN_RIGHT,   BUTTON_RIGHT_PIN,   true);
    button_register(BTN_OK,      BUTTON_OK_PIN,      true);
    button_register(BTN_CANCEL,  BUTTON_CANCEL_PIN,  true);
    button_register(BTN_START,   BUTTON_START_PIN,   true);
    button_register(BTN_STOP,    BUTTON_STOP_PIN,    true);
    
    return true;
}
