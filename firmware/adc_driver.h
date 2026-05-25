/**
 * @file adc_driver.h
 * @brief TLSR8269 ADC驱动 - 医疗级阻抗/电流检测
 * @details 支持6路ADC输入，用于电极阻抗检测和输出电流监测
 * @author 束长江
 * @version 1.0.0
 * @date 2026-05-24
 * 
 * @note 引脚定义：PB0(A0)-PB5(A5) 对应 ADC0-ADC5
 * @note 医疗级安全：12位精度，硬件过采样，EMI滤波
 */

#ifndef __ADC_DRIVER_H__
#define __ADC_DRIVER_H__

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================
 * 引脚定义
 * ============================================================ */
#define ADC_CH0_PIN         8       // PB0 - ADC0 (与PWM4复用)
#define ADC_CH1_PIN         9       // PB1 - ADC1 (与PWM5复用)
#define ADC_CH2_PIN         10      // PB2 - ADC2 (推荐用于电位器)
#define ADC_CH3_PIN         11      // PB3 - ADC3
#define ADC_CH4_PIN         12      // PB4 - ADC4
#define ADC_CH5_PIN         13      // PB5 - ADC5

#define ADC_CHANNEL_COUNT   6       // 通道总数

/* ============================================================
 * 分辨率与参考电压
 * ============================================================ */
#define ADC_RESOLUTION      12      // 12位分辨率
#define ADC_MAX_VALUE       4095    // 2^12 - 1
#define ADC_VREF_MV         3300    // 参考电压 3.3V
#define ADC_LSB_MV          0.805f  // 3.3V / 4096

/* ============================================================
 * 采样配置
 * ============================================================ */
#define ADC_OVERSAMPLE      16      // 过采样次数（提高精度）
#define ADC_SAMPLE_TIME_US  10      // 采样时间(us)
#define ADC_SETTLE_TIME_MS  5       // 通道切换稳定时间(ms)

/* ============================================================
 * 医疗级检测参数
 * ============================================================ */
// 阻抗检测（分压法：已知电阻+电极阻抗）
#define ADC_IMPEDANCE_REF_OHM   1000    // 参考电阻 1kΩ
#define ADC_IMPEDANCE_VREF_MV   3300    // 激励电压 3.3V

// 电流检测（INA219分流电阻）
#define ADC_CURRENT_SHUNT_OHM   0.1f    // 分流电阻 0.1Ω
#define ADC_CURRENT_GAIN        100     // INA219增益 100x

// 安全阈值
#define ADC_CURRENT_MAX_MA      20      // 最大允许电流 20mA
#define ADC_IMPEDANCE_MIN_OHM   500     // 最小阻抗（皮肤接触）
#define ADC_IMPEDANCE_MAX_OHM   10000   // 最大阻抗（开路）
#define ADC_VOLTAGE_MAX_MV      24000   // 最大电压 24V（需分压）

/* ============================================================
 * 通道功能分配
 * ============================================================ */
typedef enum {
    ADC_FUNC_NONE = 0,          // 未使用
    ADC_FUNC_IMPEDANCE,         // 阻抗检测
    ADC_FUNC_CURRENT,           // 电流检测
    ADC_FUNC_VOLTAGE,           // 电压检测
    ADC_FUNC_TEMPERATURE,       // 温度检测
    ADC_FUNC_POTENTIOMETER      // 电位器输入
} ADC_FunctionTypeDef;

/* ============================================================
 * 通道配置
 * ============================================================ */
typedef struct {
    uint8_t pin;                // GPIO引脚号
    ADC_FunctionTypeDef function; // 功能分配
    uint16_t oversample;        // 过采样次数
    float scale_factor;         // 缩放因子（用于校准）
    float offset_mv;            // 偏移校准(mV)
    bool enabled;               // 是否启用
} ADC_ChannelConfigTypeDef;

/* ============================================================
 * 采样结果
 * ============================================================ */
typedef struct {
    uint16_t raw;               // 原始值(0-4095)
    float voltage_mv;           // 电压(mV)
    float value;                // 物理量值（根据功能）
    uint32_t timestamp_ms;      // 采样时间戳
    bool valid;                 // 数据是否有效
} ADC_SampleTypeDef;

/* ============================================================
 * 全局状态
 * ============================================================ */
typedef struct {
    bool initialized;           // 是否已初始化
    uint32_t sample_count;      // 总采样次数
    uint32_t error_count;       // 错误次数
    uint32_t last_sample_ms;    // 最后采样时间
    bool calibration_done;      // 是否已校准
} ADC_StatusTypeDef;

/* ============================================================
 * API函数
 * ============================================================ */

/**
 * @brief 初始化ADC模块
 * @return true=成功, false=失败
 * @note 配置12位精度，连续转换模式，启用过采样
 */
bool adc_init(void);

/**
 * @brief 反初始化ADC
 */
void adc_deinit(void);

/**
 * @brief 配置ADC通道
 * @param ch 通道号(0-5)
 * @param config 配置结构体
 * @return true=成功
 */
bool adc_config_channel(uint8_t ch, const ADC_ChannelConfigTypeDef *config);

/**
 * @brief 启动单次转换
 * @param ch 通道号(0-5)
 * @return 原始ADC值(0-4095)
 */
uint16_t adc_read_raw(uint8_t ch);

/**
 * @brief 读取电压值
 * @param ch 通道号(0-5)
 * @return 电压(mV)
 */
float adc_read_voltage(uint8_t ch);

/**
 * @brief 采样并转换（带过采样和滤波）
 * @param ch 通道号(0-5)
 * @return 采样结果结构体
 */
ADC_SampleTypeDef adc_sample(uint8_t ch);

/**
 * @brief 批量采样多个通道
 * @param channels 通道号数组
 * @param count 通道数量
 * @param results 结果数组
 * @return 成功采样的通道数
 */
uint8_t adc_sample_multi(const uint8_t *channels, uint8_t count, ADC_SampleTypeDef *results);

/**
 * @brief 读取电极阻抗
 * @param ch 通道号(0-5)
 * @return 阻抗值(Ω)，0表示错误
 * @note 使用分压法测量，需要外部参考电阻
 */
uint16_t adc_read_impedance(uint8_t ch);

/**
 * @brief 读取输出电流
 * @param ch 通道号(0-5)
 * @return 电流值(mA)，负值表示错误
 * @note 需要INA219或外部分流电阻
 */
float adc_read_current(uint8_t ch);

/**
 * @brief 读取输出电压
 * @param ch 通道号(0-5)
 * @return 电压值(V)
 * @note 需要外部分压电路（24V→3.3V）
 */
float adc_read_output_voltage(uint8_t ch);

/**
 * @brief 读取电位器值
 * @param ch 通道号(0-5)
 * @return 位置值(0-10000，表示0.00%-100.00%)
 */
uint16_t adc_read_potentiometer(uint8_t ch);

/**
 * @brief 检查电极接触状态
 * @param ch 通道号(0-5)
 * @return true=接触良好, false=接触不良/开路
 */
bool adc_check_electrode(uint8_t ch);

/**
 * @brief 校准ADC（零点校准）
 * @details 将所有通道接地，测量偏移量
 * @return true=校准成功
 */
bool adc_calibrate(void);

/**
 * @brief 设置通道校准参数
 * @param ch 通道号
 * @param scale 缩放因子
 * @param offset_mv 偏移量(mV)
 */
void adc_set_calibration(uint8_t ch, float scale, float offset_mv);

/**
 * @brief 启动连续扫描模式
 * @param channels 要扫描的通道数组
 * @param count 通道数量
 * @param interval_ms 扫描间隔(ms)
 * @return true=成功
 */
bool adc_start_scan(const uint8_t *channels, uint8_t count, uint16_t interval_ms);

/**
 * @brief 停止连续扫描
 */
void adc_stop_scan(void);

/**
 * @brief 获取扫描结果
 * @param ch 通道号
 * @return 最新采样结果
 */
ADC_SampleTypeDef adc_get_scan_result(uint8_t ch);

/**
 * @brief 获取ADC状态
 * @return 状态结构体指针
 */
const ADC_StatusTypeDef* adc_get_status(void);

/**
 * @brief 设置采样回调
 * @param callback 回调函数(ch, sample)
 */
void adc_set_callback(void (*callback)(uint8_t ch, ADC_SampleTypeDef sample));

/**
 * @brief 设置安全阈值回调（过流/开路检测）
 * @param callback 回调函数(ch, type, value)
 */
void adc_set_safety_callback(void (*callback)(uint8_t ch, uint8_t type, float value));

/* ============================================================
 * 便捷函数
 * ============================================================ */

/**
 * @brief 快速配置阻抗检测通道
 * @param ch 通道号
 * @param ref_ohm 参考电阻值(Ω)
 * @return true=成功
 */
static inline bool adc_setup_impedance(uint8_t ch, uint16_t ref_ohm)
{
    ADC_ChannelConfigTypeDef config = {
        .pin = ch + 8,  // PB0-PB5
        .function = ADC_FUNC_IMPEDANCE,
        .oversample = ADC_OVERSAMPLE,
        .scale_factor = (float)ref_ohm / ADC_MAX_VALUE,
        .offset_mv = 0,
        .enabled = true
    };
    return adc_config_channel(ch, &config);
}

/**
 * @brief 快速配置电流检测通道
 * @param ch 通道号
 * @param shunt_ohm 分流电阻(Ω)
 * @return true=成功
 */
static inline bool adc_setup_current(uint8_t ch, float shunt_ohm)
{
    ADC_ChannelConfigTypeDef config = {
        .pin = ch + 8,
        .function = ADC_FUNC_CURRENT,
        .oversample = ADC_OVERSAMPLE * 2,  // 电流需要更高精度
        .scale_factor = shunt_ohm * ADC_CURRENT_GAIN,
        .offset_mv = 0,
        .enabled = true
    };
    return adc_config_channel(ch, &config);
}

/**
 * @brief 快速配置电位器通道
 * @param ch 通道号
 * @return true=成功
 */
static inline bool adc_setup_potentiometer(uint8_t ch)
{
    ADC_ChannelConfigTypeDef config = {
        .pin = ch + 8,
        .function = ADC_FUNC_POTENTIOMETER,
        .oversample = 4,  // 电位器不需要太高精度
        .scale_factor = 10000.0f / ADC_MAX_VALUE,  // 0-10000
        .offset_mv = 0,
        .enabled = true
    };
    return adc_config_channel(ch, &config);
}

#ifdef __cplusplus
}
#endif

#endif /* __ADC_DRIVER_H__ */
