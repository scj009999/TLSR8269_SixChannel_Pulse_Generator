/**
 * @file stubs.c
 * @brief 未实现函数的桩实现（临时兼容层）
 * @warning 这些函数需要在后续实现真正的功能
 */

#include "stubs.h"
#include <stddef.h>

// PWM中断处理桩
__attribute__((weak)) void pwm_irq_handler(void)
{
    // TODO: 实现PWM中断处理
}

// ADC扫描处理桩
__attribute__((weak)) void adc_scan_handler(void)
{
    // TODO: 实现ADC扫描处理
}

// 按键默认布局初始化桩
__attribute__((weak)) bool button_init_default_layout(void)
{
    // TODO: 实现默认按键布局初始化
    return true;
}

// UI更新治疗进度桩
__attribute__((weak)) void ui_update_therapy_progress(uint32_t elapsed_ms, uint32_t total_ms)
{
    // TODO: 实现UI进度更新
    (void)elapsed_ms;
    (void)total_ms;
}
