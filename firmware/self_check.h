/**************************************************************************
 * 系统自检模块头文件 — 医疗级标准
 * 功能：开机全链路检测 → 异常锁机+明确提示 → 仅全部正常才允许操作
 **************************************************************************/

#ifndef __SELF_CHECK_H__
#define __SELF_CHECK_H__

#include <stdint.h>
#include <stdbool.h>

// ============================================
// 自检结果码定义
// ============================================
#define CHECK_OK            0x00  // 全部正常
#define CHECK_FAIL_CLK      0x01  // 时钟异常
#define CHECK_FAIL_FLASH    0x02  // 存储异常
#define CHECK_FAIL_RAM      0x03  // 内存异常
#define CHECK_FAIL_BLE      0x04  // 蓝牙模块异常
#define CHECK_FAIL_ADC      0x05  // 阻抗检测电路异常
#define CHECK_FAIL_PARAM    0x06  // 参数区损坏
#define CHECK_FAIL_POWER    0x07  // 电源电压异常
#define CHECK_FAIL_PWM      0x08  // PWM输出异常
#define CHECK_FAIL_ESTOP    0x09  // 急停按钮异常

// ============================================
// 全局变量声明
// ============================================
extern uint8_t g_self_check_result;   // 自检结果码
extern bool g_system_ready;            // 系统就绪标志

// ============================================
// 函数声明
// ============================================

/**
 * @brief 执行系统全链路自检
 * @return uint8_t 自检结果码，0=正常，非0=故障
 * @note 开机时必须调用，仅全部通过才允许操作
 */
uint8_t system_self_check(void);

/**
 * @brief 获取自检结果描述字符串
 * @param code 自检结果码
 * @return const char* 结果描述
 */
const char* get_check_result_string(uint8_t code);

/**
 * @brief 系统错误处理 — 锁机+报警
 * @param error_code 错误码
 * @note 进入死循环，闪烁LED并输出错误信息
 */
void handle_system_error(uint8_t error_code);

/**
 * @brief 检测系统时钟（32M + 32K）
 * @return true=正常, false=异常
 */
static bool check_system_clock(void);

/**
 * @brief 检测Flash存储（读写+校验）
 * @return true=正常, false=异常
 */
static bool check_flash_memory(void);

/**
 * @brief 检测RAM（walking bit test）
 * @return true=正常, false=异常
 */
static bool check_ram_memory(void);

/**
 * @brief 检测电源电压（电池+3.3V）
 * @return true=正常, false=异常
 */
static bool check_power_supply(void);

/**
 * @brief 检测ADC电路
 * @return true=正常, false=异常
 */
static bool check_adc_circuit(void);

/**
 * @brief 检测PWM输出（六通道）
 * @return true=正常, false=异常
 */
static bool check_pwm_output(void);

/**
 * @brief 检测急停按钮
 * @return true=正常, false=异常
 */
static bool check_estop_button(void);

/**
 * @brief 检测蓝牙模块
 * @return true=正常, false=异常
 */
static bool check_ble_module(void);

#endif /* __SELF_CHECK_H__ */
