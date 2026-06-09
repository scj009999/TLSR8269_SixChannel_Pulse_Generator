/**
 * @file stubs.h
 * @brief 未实现函数的桩声明（临时兼容层）
 * @warning 这些函数需要在后续实现
 */

#ifndef __STUBS_H__
#define __STUBS_H__

#include <stdint.h>
#include <stdbool.h>
#include "pwm_driver.h"

#ifdef __cplusplus
extern "C" {
#endif

// PWM中断处理（应在pwm_driver.c中实现）
extern void pwm_irq_handler(void);

// ADC扫描处理（应在adc_driver.c中实现）
extern void adc_scan_handler(void);

// 按键默认布局初始化（应在button_driver.c中实现）
extern bool button_init_default_layout(void);

// UI更新治疗进度（应在ui_manager.c中实现）
extern void ui_update_therapy_progress(uint32_t elapsed_ms, uint32_t total_ms);

#ifdef __cplusplus
}
#endif

#endif /* __STUBS_H__ */
