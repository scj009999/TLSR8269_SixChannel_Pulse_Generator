/**
 * @file pwm_driver.h
 * @brief TLSR8269 六通道PWM驱动 - 医疗级脉冲发生器
 * @details 支持6路独立PWM输出，频率1Hz-8MHz，占空比0-100%
 * @author 束长江
 * @version 1.0.0
 * @date 2026-05-24
 * 
 * @note 引脚定义（基于Arduino Core修正）：
 *       CH0: PA0(PWM0), CH1: PA1(PWM1), CH2: PA2(PWM2)
 *       CH3: PA3(PWM3), CH4: PB0(PWM4), CH5: PB1(PWM5)
 * @note 医疗级安全：硬件限流、阻抗检测、急停响应<10ms
 */

#ifndef __PWM_DRIVER_H__
#define __PWM_DRIVER_H__

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================
 * 引脚定义（修正版 - 基于Arduino Core）
 * ============================================================ */
#define PWM_CH0_PIN         0       // PA0 - PWM0
#define PWM_CH1_PIN         1       // PA1 - PWM1
#define PWM_CH2_PIN         2       // PA2 - PWM2
#define PWM_CH3_PIN         3       // PA3 - PWM3
#define PWM_CH4_PIN         8       // PB0 - PWM4
#define PWM_CH5_PIN         9       // PB1 - PWM5

#define PWM_CHANNEL_COUNT   6       // 通道总数

/* ============================================================
 * 频率范围
 * ============================================================ */
#define PWM_FREQ_MIN        1.0f    // 1 Hz（医疗级低频刺激）
#define PWM_FREQ_MAX        8000000.0f  // 8 MHz
#define PWM_FREQ_DEFAULT    10000.0f    // 默认10kHz

/* ============================================================
 * 占空比范围
 * ============================================================ */
#define PWM_DUTY_MIN        0       // 0%
#define PWM_DUTY_MAX        10000   // 100.00% (精度0.01%)
#define PWM_DUTY_DEFAULT    5000    // 50.00%

/* ============================================================
 * 医疗级安全限制
 * ============================================================ */
#define PWM_CURRENT_LIMIT_MA    20      // 硬件电流限制 20mA（单通道）
#define PWM_VOLTAGE_LIMIT_V     24.0f   // 电压限制 24V
#define PWM_IMPEDANCE_MIN_OHM   500     // 最小阻抗 500Ω（皮肤接触检测）
#define PWM_IMPEDANCE_MAX_OHM   10000   // 最大阻抗 10kΩ（开路检测）
#define PWM_EMERGENCY_STOP_MS   10      // 急停响应时间 <10ms

/* ============================================================
 * 通道状态
 * ============================================================ */
typedef enum {
    PWM_STATE_IDLE = 0,         // 空闲
    PWM_STATE_READY,            // 就绪（参数已设置）
    PWM_STATE_RUNNING,          // 运行中
    PWM_STATE_PAUSED,           // 暂停
    PWM_STATE_FAULT             // 故障（过流/开路/短路）
} PWM_StateTypeDef;

/* ============================================================
 * 故障类型
 * ============================================================ */
typedef enum {
    PWM_FAULT_NONE = 0,         // 无故障
    PWM_FAULT_OVERCURRENT,      // 过流
    PWM_FAULT_OPEN_CIRCUIT,     // 开路（电极脱落）
    PWM_FAULT_SHORT,            // 短路
    PWM_FAULT_OVERVOLTAGE,      // 过压
    PWM_FAULT_OVERTEMP,         // 过热
    PWM_FAULT_IMPEDANCE_LOW,    // 阻抗过低
    PWM_FAULT_IMPEDANCE_HIGH    // 阻抗过高
} PWM_FaultTypeDef;

/* ============================================================
 * 通道配置结构
 * ============================================================ */
typedef struct {
    float frequency_hz;         // 频率(Hz)
    uint16_t duty;              // 占空比(0-10000，表示0.00%-100.00%)
    uint16_t pulse_width_us;    // 脉宽(us)，用于单脉冲模式
    bool enabled;               // 是否启用
    bool inverted;              // 极性反转
} PWM_ChannelConfigTypeDef;

/* ============================================================
 * 通道状态结构
 * ============================================================ */
typedef struct {
    PWM_StateTypeDef state;     // 状态
    PWM_FaultTypeDef fault;     // 故障类型
    float current_ma;           // 当前电流(mA)
    float voltage_v;            // 当前电压(V)
    uint16_t impedance_ohm;     // 阻抗(Ω)
    uint32_t runtime_ms;        // 运行时间(ms)
    uint32_t pulse_count;       // 脉冲计数
} PWM_ChannelStatusTypeDef;

/* ============================================================
 * 全局状态
 * ============================================================ */
typedef struct {
    bool initialized;           // 是否已初始化
    bool emergency_stop;        // 急停标志
    uint32_t total_runtime_ms;  // 总运行时间
    uint8_t active_channels;    // 活动通道数
    uint32_t fault_count;       // 故障次数（医疗级：记录用于分析）
} PWM_GlobalStatusTypeDef;

/* ============================================================
 * API函数
 * ============================================================ */

/**
 * @brief 初始化PWM模块
 * @return true=成功, false=失败
 * @note 配置6通道PWM，默认频率10kHz，占空比50%，全部停止
 */
bool pwm_init(void);

/**
 * @brief 反初始化PWM
 */
void pwm_deinit(void);

/**
 * @brief 配置通道参数
 * @param ch 通道号(0-5)
 * @param config 配置结构体
 * @return true=成功, false=失败
 */
bool pwm_config_channel(uint8_t ch, const PWM_ChannelConfigTypeDef *config);

/**
 * @brief 设置通道频率
 * @param ch 通道号(0-5)
 * @param freq_hz 频率(Hz)，范围1-8MHz
 * @return true=成功, false=失败
 */
bool pwm_set_frequency(uint8_t ch, float freq_hz);

/**
 * @brief 设置通道占空比
 * @param ch 通道号(0-5)
 * @param duty 占空比(0-10000，表示0.00%-100.00%)
 * @return true=成功, false=失败
 */
bool pwm_set_duty(uint8_t ch, uint16_t duty);

/**
 * @brief 启动通道
 * @param ch 通道号(0-5)
 * @return true=成功, false=失败
 */
bool pwm_start_channel(uint8_t ch);

/**
 * @brief 停止通道
 * @param ch 通道号(0-5)
 */
void pwm_stop_channel(uint8_t ch);

/**
 * @brief 启动所有通道
 */
void pwm_start_all(void);

/**
 * @brief 停止所有通道
 */
void pwm_stop_all(void);

/**
 * @brief 暂停所有通道（保持配置）
 */
void pwm_pause_all(void);

/**
 * @brief 恢复所有通道
 */
void pwm_resume_all(void);

/**
 * @brief 获取通道状态
 * @param ch 通道号(0-5)
 * @return 状态结构体指针
 */
const PWM_ChannelStatusTypeDef* pwm_get_channel_status(uint8_t ch);

/**
 * @brief 获取全局状态
 * @return 全局状态结构体指针
 */
const PWM_GlobalStatusTypeDef* pwm_get_global_status(void);

/**
 * @brief 急停（医疗级安全）
 * @details 立即停止所有通道，设置急停标志
 * @note 响应时间<10ms
 */
void pwm_emergency_stop(void);

/**
 * @brief 清除急停
 * @details 需要手动确认后才能恢复
 */
void pwm_clear_emergency(void);

/**
 * @brief 清除通道故障
 * @param ch 通道号(0-5)
 */
void pwm_clear_fault(uint8_t ch);

/**
 * @brief 设置全局电流限制
 * @param limit_ma 限制值(mA)
 */
void pwm_set_current_limit(uint16_t limit_ma);

/**
 * @brief 获取通道当前电流（通过ADC或INA219）
 * @param ch 通道号(0-5)
 * @return 电流值(mA)
 */
float pwm_read_current(uint8_t ch);

/**
 * @brief 获取通道阻抗
 * @param ch 通道号(0-5)
 * @return 阻抗值(Ω)
 */
uint16_t pwm_read_impedance(uint8_t ch);

/**
 * @brief 检查电极接触（阻抗检测）
 * @param ch 通道号(0-5)
 * @return true=接触良好, false=接触不良
 */
bool pwm_check_electrode(uint8_t ch);

/**
 * @brief 单次脉冲触发（用于测试）
 * @param ch 通道号(0-5)
 * @param pulse_width_us 脉宽(us)
 */
void pwm_single_pulse(uint8_t ch, uint16_t pulse_width_us);

/**
 * @brief 设置脉冲计数回调
 * @param callback 回调函数(ch, count)
 */
void pwm_set_pulse_callback(void (*callback)(uint8_t ch, uint32_t count));

/**
 * @brief 设置故障回调
 * @param callback 回调函数(ch, fault_type)
 */
void pwm_set_fault_callback(void (*callback)(uint8_t ch, PWM_FaultTypeDef fault));

/* ============================================================
 * 便捷函数
 * ============================================================ */

/**
 * @brief 快速配置通道（一键设置）
 * @param ch 通道号
 * @param freq_hz 频率
 * @param duty 占空比(0-10000)
 * @param enable 是否立即启动
 * @return true=成功
 */
static inline bool pwm_setup(uint8_t ch, float freq_hz, uint16_t duty, bool enable)
{
    PWM_ChannelConfigTypeDef config = {
        .frequency_hz = freq_hz,
        .duty = duty,
        .pulse_width_us = 0,
        .enabled = enable,
        .inverted = false
    };
    
    if (!pwm_config_channel(ch, &config)) {
        return false;
    }
    
    if (enable) {
        return pwm_start_channel(ch);
    }
    
    return true;
}

#ifdef __cplusplus
}
#endif

#endif /* __PWM_DRIVER_H__ */
