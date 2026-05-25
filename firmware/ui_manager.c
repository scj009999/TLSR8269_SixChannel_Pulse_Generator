/**************************************************************************
 * @file ui_manager.c
 * @brief TLSR8269 用户界面管理实现
 * @author 全国首个第三方TLSR8269 C库
 * @note 治疗设备LCD界面，支持菜单导航和参数编辑
 **************************************************************************/

#include "ui_manager.h"
#include "main_master.h"
#include <stdio.h>
#include <string.h>

//==========================================================================
// 一、全局变量
//==========================================================================
static UI_StateTypeDef ui_state;
static bool ui_initialized = false;

// 菜单文本
static const char *menu_items[MENU_ITEM_MAX] = {
    "Start Therapy",
    "Channel Config",
    "Select Scheme",
    "Settings",
    "About"
};

// 通道参数缓存
static struct {
    bool enabled;
    float freq;
    uint32_t width;
    uint8_t intensity;
} channel_display[6];

// 运行状态
static struct {
    bool running;
    uint32_t elapsed_sec;
    uint32_t total_sec;
    uint8_t progress;
} run_status;

//==========================================================================
// 二、初始化
//==========================================================================
bool ui_init(void)
{
    if (ui_initialized) {
        return true;
    }
    
    // 初始化LCD
    if (!lcd_init()) {
        return false;
    }
    
    // 初始化按键
    if (!button_init_default_layout()) {
        lcd_deinit();
        return false;
    }
    
    // 初始化状态
    memset(&ui_state, 0, sizeof(UI_StateTypeDef));
    ui_state.current_mode = UI_MODE_MAIN;
    ui_state.refresh_needed = true;
    
    // 注册按键回调
    button_set_callback(ui_handle_button);
    
    ui_initialized = true;
    
    // 绘制初始界面
    ui_refresh();
    
    return true;
}

void ui_deinit(void)
{
    lcd_deinit();
    button_deinit();
    ui_initialized = false;
}

//==========================================================================
// 三、模式切换
//==========================================================================
void ui_set_mode(UI_ModeTypeDef mode)
{
    if (mode == ui_state.current_mode) {
        return;
    }
    
    ui_state.previous_mode = ui_state.current_mode;
    ui_state.current_mode = mode;
    ui_state.refresh_needed = true;
    
    // 清除屏幕
    lcd_clear();
}

UI_ModeTypeDef ui_get_mode(void)
{
    return ui_state.current_mode;
}

void ui_go_back(void)
{
    if (ui_state.previous_mode != UI_MODE_MAIN) {
        ui_set_mode(ui_state.previous_mode);
    } else {
        ui_set_mode(UI_MODE_MAIN);
    }
}

//==========================================================================
// 四、界面绘制
//==========================================================================
void ui_refresh(void)
{
    if (!ui_state.refresh_needed) {
        return;
    }
    
    switch (ui_state.current_mode) {
        case UI_MODE_MAIN:
            ui_draw_main_screen();
            break;
        case UI_MODE_MENU:
            ui_draw_menu();
            break;
        case UI_MODE_CHANNEL_EDIT:
            ui_draw_channel_edit(ui_state.channel_index);
            break;
        case UI_MODE_SCHEME_SELECT:
            ui_draw_scheme_select();
            break;
        case UI_MODE_RUNNING:
            ui_draw_running_screen();
            break;
        case UI_MODE_FAULT:
            // 故障码由外部传入
            break;
        case UI_MODE_SETTINGS:
            ui_draw_settings();
            break;
        default:
            break;
    }
    
    lcd_refresh();
    ui_state.refresh_needed = false;
    ui_state.last_refresh = get_system_tick();
}

// 4.1 主界面
void ui_draw_main_screen(void)
{
    lcd_clear();
    
    // 标题
    lcd_draw_title("Pulse Therapy");
    
    // 通道状态概览
    lcd_draw_string(0, 14, "Channels:", true);
    
    for (uint8_t i = 0; i < 6; i++) {
        uint8_t y = 24 + i * 6;
        char buf[32];
        
        if (channel_display[i].enabled) {
            snprintf(buf, sizeof(buf), "CH%d:%3.1fHz %3d%%", 
                     i, channel_display[i].freq, channel_display[i].intensity);
            lcd_draw_string(0, y, buf, true);
        } else {
            snprintf(buf, sizeof(buf), "CH%d: OFF", i);
            lcd_draw_string(0, y, buf, false);
        }
    }
    
    // 底部提示
    lcd_draw_string(0, 56, "[OK]Menu [Start]Run", true);
}

// 4.2 菜单界面
void ui_draw_menu(void)
{
    lcd_clear();
    
    // 标题
    lcd_draw_title("Main Menu");
    
    // 菜单项
    for (uint8_t i = 0; i < MENU_ITEM_MAX; i++) {
        uint8_t y = 14 + i * 10;
        lcd_draw_menu_item(y, menu_items[i], (i == ui_state.menu_index));
    }
    
    // 底部提示
    lcd_draw_string(0, 56, "[Up/Down]Nav [OK]Sel", true);
}

// 4.3 通道编辑界面
void ui_draw_channel_edit(uint8_t ch)
{
    lcd_clear();
    
    // 标题
    char title[16];
    snprintf(title, sizeof(title), "Channel %d", ch);
    lcd_draw_title(title);
    
    // 参数显示
    lcd_draw_string(0, 14, "Enable:", true);
    lcd_draw_string(70, 14, channel_display[ch].enabled ? "ON " : "OFF", true);
    
    lcd_draw_string(0, 24, "Freq:", true);
    lcd_draw_float(50, 24, channel_display[ch].freq, 1, true);
    lcd_draw_string(90, 24, "Hz", true);
    
    lcd_draw_string(0, 34, "Width:", true);
    lcd_draw_number(50, 34, channel_display[ch].width, true);
    lcd_draw_string(80, 34, "us", true);
    
    lcd_draw_string(0, 44, "Intensity:", true);
    lcd_draw_progress_bar(70, 44, 50, channel_display[ch].intensity);
    
    // 底部提示
    lcd_draw_string(0, 56, "[Up/Down]Adj [OK]Save", true);
}

// 4.4 方案选择界面
void ui_draw_scheme_select(void)
{
    lcd_clear();
    
    lcd_draw_title("Select Scheme");
    
    // 显示预设方案
    const char *schemes[] = {
        "Default",
        "Parkinson A",
        "Parkinson B",
        "Sleep Aid",
        "Custom 1",
        "Custom 2"
    };
    
    for (uint8_t i = 0; i < 6; i++) {
        uint8_t y = 14 + i * 8;
        lcd_draw_menu_item(y, schemes[i], (i == ui_state.scheme_index));
    }
    
    lcd_draw_string(0, 56, "[Up/Down]Nav [OK]Load", true);
}

// 4.5 运行中界面
void ui_draw_running_screen(void)
{
    lcd_clear();
    
    // 标题
    lcd_draw_title("Therapy Running");
    
    // 进度条
    lcd_draw_progress_bar(10, 16, 108, run_status.progress);
    
    // 时间显示
    char time_str[32];
    uint32_t elapsed_min = run_status.elapsed_sec / 60;
    uint32_t elapsed_sec = run_status.elapsed_sec % 60;
    uint32_t total_min = run_status.total_sec / 60;
    uint32_t total_sec = run_status.total_sec % 60;
    
    snprintf(time_str, sizeof(time_str), "%02lu:%02lu / %02lu:%02lu",
             elapsed_min, elapsed_sec, total_min, total_sec);
    lcd_draw_string(20, 28, time_str, true);
    
    // 通道状态
    lcd_draw_string(0, 40, "Active:", true);
    uint8_t active_count = 0;
    for (uint8_t i = 0; i < 6; i++) {
        if (channel_display[i].enabled) {
            char ch_str[4];
            snprintf(ch_str, sizeof(ch_str), "C%d", i);
            lcd_draw_string(50 + active_count * 18, 40, ch_str, true);
            active_count++;
        }
    }
    
    // 底部提示
    lcd_draw_string(0, 56, "[Stop]Stop Therapy", true);
}

// 4.6 故障界面
void ui_draw_fault_screen(uint8_t fault_code)
{
    lcd_clear();
    
    // 标题（反色警告）
    lcd_fill_rect(0, 0, LCD_WIDTH, 12, true);
    lcd_draw_string(30, 2, "FAULT!", false);
    
    // 故障信息
    const char *fault_msgs[] = {
        "Unknown Error",
        "Over Current!",
        "Over Voltage!",
        "Impedance Low!",
        "Impedance High!",
        "Over Temperature!",
        "Emergency Stop!",
        "Comm Timeout!",
        "Invalid Param!"
    };
    
    if (fault_code < sizeof(fault_msgs) / sizeof(fault_msgs[0])) {
        lcd_draw_string(0, 20, fault_msgs[fault_code], true);
    }
    
    lcd_draw_string(0, 36, "Please check:", true);
    lcd_draw_string(0, 46, "- Electrode contact", true);
    lcd_draw_string(0, 54, "- Power supply", true);
    
    lcd_refresh();
}

// 4.7 设置界面
void ui_draw_settings(void)
{
    lcd_clear();
    
    lcd_draw_title("Settings");
    
    const char *settings[] = {
        "Contrast",
        "Beep",
        "Language",
        "Factory Reset",
        "Version Info"
    };
    
    for (uint8_t i = 0; i < 5; i++) {
        uint8_t y = 14 + i * 10;
        lcd_draw_menu_item(y, settings[i], false);
    }
    
    lcd_draw_string(0, 56, "[Cancel]Back", true);
}

//==========================================================================
// 五、事件处理
//==========================================================================
void ui_handle_button(uint8_t btn_id, Button_EventTypeDef event)
{
    if (event == BUTTON_EVENT_NONE) {
        return;
    }
    
    ui_state.refresh_needed = true;
    
    switch (ui_state.current_mode) {
        case UI_MODE_MAIN:
            if (btn_id == BTN_OK && event == BUTTON_EVENT_SHORT_PRESS) {
                ui_set_mode(UI_MODE_MENU);
            } else if (btn_id == BTN_START && event == BUTTON_EVENT_SHORT_PRESS) {
                // 启动治疗
                ui_set_mode(UI_MODE_RUNNING);
            }
            break;
            
        case UI_MODE_MENU:
            if (btn_id == BTN_UP && event == BUTTON_EVENT_SHORT_PRESS) {
                if (ui_state.menu_index > 0) {
                    ui_state.menu_index--;
                }
            } else if (btn_id == BTN_DOWN && event == BUTTON_EVENT_SHORT_PRESS) {
                if (ui_state.menu_index < MENU_ITEM_MAX - 1) {
                    ui_state.menu_index++;
                }
            } else if (btn_id == BTN_OK && event == BUTTON_EVENT_SHORT_PRESS) {
                switch (ui_state.menu_index) {
                    case MENU_ITEM_START:
                        ui_set_mode(UI_MODE_RUNNING);
                        break;
                    case MENU_ITEM_CHANNEL_CFG:
                        ui_state.channel_index = 0;
                        ui_set_mode(UI_MODE_CHANNEL_EDIT);
                        break;
                    case MENU_ITEM_SCHEME_SELECT:
                        ui_set_mode(UI_MODE_SCHEME_SELECT);
                        break;
                    case MENU_ITEM_SETTINGS:
                        ui_set_mode(UI_MODE_SETTINGS);
                        break;
                    case MENU_ITEM_ABOUT:
                        // 显示关于信息
                        ui_show_message("About", "Pulse Therapy v1.0", 3000);
                        break;
                }
            } else if (btn_id == BTN_CANCEL && event == BUTTON_EVENT_SHORT_PRESS) {
                ui_go_back();
            }
            break;
            
        case UI_MODE_CHANNEL_EDIT:
            if (btn_id == BTN_UP && event == BUTTON_EVENT_SHORT_PRESS) {
                // 增加参数
                // TODO: 根据当前编辑项调整参数
            } else if (btn_id == BTN_DOWN && event == BUTTON_EVENT_SHORT_PRESS) {
                // 减少参数
            } else if (btn_id == BTN_OK && event == BUTTON_EVENT_SHORT_PRESS) {
                // 保存并下一个通道
                if (ui_state.channel_index < 5) {
                    ui_state.channel_index++;
                } else {
                    ui_go_back();
                }
            } else if (btn_id == BTN_CANCEL && event == BUTTON_EVENT_SHORT_PRESS) {
                ui_go_back();
            }
            break;
            
        case UI_MODE_SCHEME_SELECT:
            if (btn_id == BTN_UP && event == BUTTON_EVENT_SHORT_PRESS) {
                if (ui_state.scheme_index > 0) {
                    ui_state.scheme_index--;
                }
            } else if (btn_id == BTN_DOWN && event == BUTTON_EVENT_SHORT_PRESS) {
                if (ui_state.scheme_index < 5) {
                    ui_state.scheme_index++;
                }
            } else if (btn_id == BTN_OK && event == BUTTON_EVENT_SHORT_PRESS) {
                // 加载方案
                ui_go_back();
            } else if (btn_id == BTN_CANCEL && event == BUTTON_EVENT_SHORT_PRESS) {
                ui_go_back();
            }
            break;
            
        case UI_MODE_RUNNING:
            if (btn_id == BTN_STOP && event == BUTTON_EVENT_SHORT_PRESS) {
                // 停止治疗
                ui_set_mode(UI_MODE_MAIN);
            }
            break;
            
        case UI_MODE_SETTINGS:
            if (btn_id == BTN_CANCEL && event == BUTTON_EVENT_SHORT_PRESS) {
                ui_go_back();
            }
            break;
            
        default:
            break;
    }
}

//==========================================================================
// 六、主处理循环
//==========================================================================
void ui_process(void)
{
    if (!ui_initialized) {
        return;
    }
    
    // 扫描按键
    button_scan();
    
    // 刷新显示（限制10fps）
    uint32_t now = get_system_tick();
    if (now - ui_state.last_refresh >= 100) {  // 100ms = 10fps
        ui_refresh();
    }
}

//==========================================================================
// 七、数据更新
//==========================================================================
void ui_update_channel_status(uint8_t ch, bool enabled, float freq, uint8_t intensity)
{
    if (ch >= 6) return;
    
    channel_display[ch].enabled = enabled;
    channel_display[ch].freq = freq;
    channel_display[ch].intensity = intensity;
    
    if (ui_state.current_mode == UI_MODE_MAIN) {
        ui_state.refresh_needed = true;
    }
}

void ui_update_progress(uint8_t percent)
{
    run_status.progress = percent;
    
    if (ui_state.current_mode == UI_MODE_RUNNING) {
        ui_state.refresh_needed = true;
    }
}

void ui_update_time(uint32_t elapsed_sec, uint32_t total_sec)
{
    run_status.elapsed_sec = elapsed_sec;
    run_status.total_sec = total_sec;
    
    if (ui_state.current_mode == UI_MODE_RUNNING) {
        ui_state.refresh_needed = true;
    }
}

void ui_show_message(const char *title, const char *msg, uint16_t timeout_ms)
{
    lcd_clear();
    
    // 标题栏
    lcd_fill_rect(0, 0, LCD_WIDTH, 12, true);
    lcd_draw_string((LCD_WIDTH - strlen(title) * 6) / 2, 2, title, false);
    
    // 消息内容
    lcd_draw_string(0, 20, msg, true);
    
    lcd_refresh();
    
    // 延时
    delay_ms(timeout_ms);
    
    // 恢复之前界面
    ui_state.refresh_needed = true;
    ui_refresh();
}

void ui_show_alert(const char *msg)
{
    ui_show_message("ALERT", msg, 2000);
}
