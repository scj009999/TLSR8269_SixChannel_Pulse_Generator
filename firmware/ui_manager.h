/**************************************************************************
 * @file ui_manager.h
 * @brief TLSR8269 用户界面管理头文件
 * @author 全国首个第三方TLSR8269 C库
 * @note 治疗设备LCD界面，菜单系统，参数显示
 **************************************************************************/

#ifndef __UI_MANAGER_H
#define __UI_MANAGER_H

#include <stdint.h>
#include <stdbool.h>
#include "lcd_driver.h"
#include "button_driver.h"

//==========================================================================
// 一、界面模式
//==========================================================================
typedef enum {
    UI_MODE_MAIN = 0,       // 主界面
    UI_MODE_MENU,           // 菜单界面
    UI_MODE_CHANNEL_EDIT,   // 通道编辑
    UI_MODE_SCHEME_SELECT,  // 方案选择
    UI_MODE_RUNNING,        // 运行中
    UI_MODE_FAULT,          // 故障显示
    UI_MODE_SETTINGS        // 设置
} UI_ModeTypeDef;

//==========================================================================
// 二、菜单项
//==========================================================================
typedef enum {
    MENU_ITEM_START = 0,
    MENU_ITEM_CHANNEL_CFG,
    MENU_ITEM_SCHEME_SELECT,
    MENU_ITEM_SETTINGS,
    MENU_ITEM_ABOUT,
    MENU_ITEM_MAX
} Menu_ItemTypeDef;

//==========================================================================
// 三、界面状态
//==========================================================================
typedef struct {
    UI_ModeTypeDef current_mode;
    UI_ModeTypeDef previous_mode;
    uint8_t menu_index;
    uint8_t channel_index;
    uint8_t scheme_index;
    bool refresh_needed;
    uint32_t last_refresh;
} UI_StateTypeDef;

//==========================================================================
// 四、API函数声明
//==========================================================================

// 初始化
bool ui_init(void);
void ui_deinit(void);

// 模式切换
void ui_set_mode(UI_ModeTypeDef mode);
UI_ModeTypeDef ui_get_mode(void);
void ui_go_back(void);

// 界面绘制
void ui_refresh(void);
void ui_draw_main_screen(void);
void ui_draw_menu(void);
void ui_draw_channel_edit(uint8_t ch);
void ui_draw_scheme_select(void);
void ui_draw_running_screen(void);
void ui_draw_fault_screen(uint8_t fault_code);
void ui_draw_settings(void);

// 事件处理
void ui_handle_button(uint8_t btn_id, Button_EventTypeDef event);
void ui_process(void);

// 数据更新
void ui_update_channel_status(uint8_t ch, bool enabled, float freq, uint8_t intensity);
void ui_update_progress(uint8_t percent);
void ui_update_time(uint32_t elapsed_sec, uint32_t total_sec);
void ui_show_message(const char *title, const char *msg, uint16_t timeout_ms);
void ui_show_alert(const char *msg);

#endif /* __UI_MANAGER_H */
