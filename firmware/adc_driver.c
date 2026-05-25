/**
 * @file adc_driver.c
 * @brief TLSR8269 ADC驱动实现
 * @details 12位精度，支持过采样和医疗级安全检测
 * @author 束长江
 * @version 1.0.0
 * @date 2026-05-24
 */

#include "adc_driver.h"
#include "tlsr8269_reg.h"
#include <string.h>

/* ============================================================
 * TLSR8269 ADC寄存器定义
 * ============================================================ */
#define ADC_BASE_ADDR       0x800200  // ADC模块基地址

// ADC控制寄存器
#define REG_ADC_CTRL        (ADC_BASE_ADDR + 0x00)
#define REG_ADC_CFG         (ADC_BASE_ADDR + 0x01)
#define REG_ADC_DATA        (ADC_BASE_ADDR + 0x02)  // 12位数据
#define REG_ADC_CH_SEL      (ADC_BASE_ADDR + 0x04)
#define REG_ADC_INT         (ADC_BASE_ADDR + 0x05)
#define REG_ADC_CLK         (ADC_BASE_ADDR + 0x06)

// 控制位
#define ADC_CTRL_EN         (1 << 0)    // ADC使能
#define ADC_CTRL_START      (1 << 1)    // 启动转换
#define ADC_CTRL_CONT       (1 << 2)    // 连续模式
#define ADC_CTRL_DMA_EN     (1 << 3)    // DMA使能

// 配置位
#define ADC_CFG_RES_12B     (0 << 0)    // 12位分辨率
#define ADC_CFG_RES_14B     (1 << 0)    // 14位分辨率（过采样）
#define ADC_CFG_RES_16B     (2 << 0)    // 16位分辨率（过采样）
#define ADC_CFG_REF_INT     (0 << 2)    // 内部参考电压
#define ADC_CFG_REF_EXT     (1 << 2)    // 外部参考电压
#define ADC_CFG_ALIGN_R     (0 << 3)    // 右对齐
#define ADC_CFG_ALIGN_L     (1 << 3)    // 左对齐

// 通道选择（对应GPIO）
#define ADC_CH_GPIO_PB0     0x00    // PB0
#define ADC_CH_GPIO_PB1     0x01    // PB1
#define ADC_CH_GPIO_PB2     0x02    // PB2
#define ADC_CH_GPIO_PB3     0x03    // PB3
#define ADC_CH_GPIO_PB4     0x04    // PB4
#define ADC_CH_GPIO_PB5     0x05    // PB5
#define ADC_CH_GPIO_PB6     0x06    // PB6
#define ADC_CH_GPIO_PB7     0x07    // PB7
#define ADC_CH_TEMP         0x08    // 内部温度传感器
#define ADC_CH_VBAT         0x09    // 电池电压

// 中断标志
#define ADC_INT_DONE        (1 << 0)    // 转换完成
#define ADC_INT_OVR         (1 << 1)    // 溢出

/* ============================================================
 * GPIO配置
 * ============================================================ */
#define GPIO_BASE_ADDR      0x800580

#define REG_PB_FUNC         (GPIO_BASE_ADDR + 0x4B)
#define REG_PB_OUTPUT       (GPIO_BASE_ADDR + 0x4C)
#define REG_PB_INPUT        (GPIO_BASE_ADDR + 0x4D)
#define REG_PB_PULLUP       (GPIO_BASE_ADDR + 0x4E)
#define REG_PB_OEN          (GPIO_BASE_ADDR + 0x4F)

/* ============================================================
 * 私有变量
 * ============================================================ */
static ADC_ChannelConfigTypeDef adc_config[ADC_CHANNEL_COUNT];
static ADC_SampleTypeDef adc_sample_buf[ADC_CHANNEL_COUNT];
static ADC_StatusTypeDef adc_status = {0};

static void (*adc_callback)(uint8_t ch, ADC_SampleTypeDef sample) = NULL;
static void (*safety_callback)(uint8_t ch, uint8_t type, float value) = NULL;

// 扫描模式变量
static bool scan_mode = false;
static uint8_t scan_channels[ADC_CHANNEL_COUNT];
static uint8_t scan_count = 0;
static uint16_t scan_interval_ms = 100;
static uint32_t last_scan_ms = 0;

// 通道映射：ADC通道号 → GPIO引脚
static const uint8_t adc_ch_map[ADC_CHANNEL_COUNT] = {
    ADC_CH_GPIO_PB0,  // CH0 → PB0
    ADC_CH_GPIO_PB1,  // CH1 → PB1
    ADC_CH_GPIO_PB2,  // CH2 → PB2
    ADC_CH_GPIO_PB3,  // CH3 → PB3
    ADC_CH_GPIO_PB4,  // CH4 → PB4
    ADC_CH_GPIO_PB5   // CH5 → PB5
};

/* ============================================================
 * 私有函数
 * ============================================================ */

/**
 * @brief 微秒延时
 */
static void adc_delay_us(uint32_t us)
{
    volatile uint32_t count = us * 48;
    while (count--) {
        __asm__("nop");
    }
}

/**
 * @brief 毫秒延时
 */
static void adc_delay_ms(uint32_t ms)
{
    for (uint32_t i = 0; i < ms; i++) {
        adc_delay_us(1000);
    }
}

/**
 * @brief 配置GPIO为ADC输入
 */
static void adc_gpio_init(uint8_t ch)
{
    if (ch >= ADC_CHANNEL_COUNT) return;
    
    uint8_t pin = ch;  // PB0-PB5
    
    // 禁用输出，使能输入
    uint8_t pb_oen = read_reg8(REG_PB_OEN);
    pb_oen &= ~(1 << pin);  // 输入模式
    write_reg8(REG_PB_OEN, pb_oen);
    
    uint8_t pb_input = read_reg8(REG_PB_INPUT);
    pb_input |= (1 << pin);  // 使能输入
    write_reg8(REG_PB_INPUT, pb_input);
    
    // 禁用上拉（ADC输入不需要）
    uint8_t pb_pullup = read_reg8(REG_PB_PULLUP);
    pb_pullup &= ~(1 << pin);
    write_reg8(REG_PB_PULLUP, pb_pullup);
    
    // 切换到GPIO功能（ADC复用）
    uint8_t pb_func = read_reg8(REG_PB_FUNC);
    pb_func &= ~(1 << pin);  // 设为GPIO功能（ADC通过其他寄存器选择）
    write_reg8(REG_PB_FUNC, pb_func);
}

/**
 * @brief 启动单次转换并等待完成
 */
static uint16_t adc_convert_single(uint8_t adc_ch)
{
    // 选择通道
    write_reg8(REG_ADC_CH_SEL, adc_ch);
    
    // 启动转换
    write_reg8(REG_ADC_CTRL, ADC_CTRL_EN | ADC_CTRL_START);
    
    // 等待完成（轮询）
    uint32_t timeout = 1000;  // 超时计数
    while (!(read_reg8(REG_ADC_INT) & ADC_INT_DONE)) {
        if (--timeout == 0) {
            adc_status.error_count++;
            return 0;  // 超时错误
        }
    }
    
    // 清除中断标志
    write_reg8(REG_ADC_INT, ADC_INT_DONE);
    
    // 读取数据（12位，右对齐）
    uint16_t data = read_reg16(REG_ADC_DATA) & 0x0FFF;
    
    return data;
}

/**
 * @brief 过采样读取（提高精度）
 */
static uint16_t adc_oversample_read(uint8_t adc_ch, uint16_t times)
{
    if (times == 0) times = 1;
    if (times > 256) times = 256;  // 限制最大次数
    
    uint32_t sum = 0;
    uint16_t valid_samples = 0;
    
    for (uint16_t i = 0; i < times; i++) {
        uint16_t sample = adc_convert_single(adc_ch);
        
        // 简单滤波：丢弃0值（可能表示错误）
        if (sample > 0 || times == 1) {
            sum += sample;
            valid_samples++;
        }
        
        // 采样间隔
        if (times > 1) {
            adc_delay_us(ADC_SAMPLE_TIME_US);
        }
    }
    
    if (valid_samples == 0) {
        return 0;
    }
    
    // 计算平均值
    return (uint16_t)(sum / valid_samples);
}

/**
 * @brief 原始值转电压
 */
static float adc_raw_to_voltage(uint16_t raw)
{
    return (float)raw * ADC_LSB_MV;
}

/**
 * @brief 电压转原始值
 */
static uint16_t adc_voltage_to_raw(float voltage_mv)
{
    if (voltage_mv < 0) return 0;
    if (voltage_mv > ADC_VREF_MV) return ADC_MAX_VALUE;
    
    return (uint16_t)((voltage_mv / ADC_VREF_MV) * ADC_MAX_VALUE);
}

/**
 * @brief 安全检查（过流/开路）
 */
static void adc_safety_check(uint8_t ch, float current_ma, uint16_t impedance_ohm)
{
    if (safety_callback == NULL) return;
    
    // 过流检测
    if (current_ma > ADC_CURRENT_MAX_MA) {
        safety_callback(ch, 1, current_ma);  // type=1: 过流
    }
    
    // 开路检测（阻抗过高）
    if (impedance_ohm > ADC_IMPEDANCE_MAX_OHM) {
        safety_callback(ch, 2, (float)impedance_ohm);  // type=2: 开路
    }
    
    // 短路检测（阻抗过低）
    if (impedance_ohm > 0 && impedance_ohm < ADC_IMPEDANCE_MIN_OHM) {
        safety_callback(ch, 3, (float)impedance_ohm);  // type=3: 短路
    }
}

/* ============================================================
 * API实现
 * ============================================================ */

bool adc_init(void)
{
    if (adc_status.initialized) {
        return true;
    }
    
    // 初始化状态
    memset(&adc_status, 0, sizeof(adc_status));
    memset(adc_config, 0, sizeof(adc_config));
    memset(adc_sample_buf, 0, sizeof(adc_sample_buf));
    
    // 配置ADC时钟（默认分频，目标采样率适中）
    write_reg8(REG_ADC_CLK, 0x04);  // 48MHz / 5 = 9.6MHz ADC时钟
    
    // 配置ADC：12位分辨率，内部参考，右对齐
    write_reg8(REG_ADC_CFG, ADC_CFG_RES_12B | ADC_CFG_REF_INT | ADC_CFG_ALIGN_R);
    
    // 清除中断
    write_reg8(REG_ADC_INT, 0xFF);
    
    // 初始化所有通道的GPIO
    for (uint8_t ch = 0; ch < ADC_CHANNEL_COUNT; ch++) {
        adc_gpio_init(ch);
        
        // 默认配置
        adc_config[ch].pin = ch + 8;  // PB0-PB5
        adc_config[ch].function = ADC_FUNC_NONE;
        adc_config[ch].oversample = ADC_OVERSAMPLE;
        adc_config[ch].scale_factor = 1.0f;
        adc_config[ch].offset_mv = 0;
        adc_config[ch].enabled = false;
    }
    
    adc_status.initialized = true;
    
    return true;
}

void adc_deinit(void)
{
    if (!adc_status.initialized) {
        return;
    }
    
    // 禁用ADC
    write_reg8(REG_ADC_CTRL, 0);
    
    // 恢复GPIO为默认状态
    for (uint8_t ch = 0; ch < ADC_CHANNEL_COUNT; ch++) {
        uint8_t pin = ch;
        
        uint8_t pb_oen = read_reg8(REG_PB_OEN);
        pb_oen |= (1 << pin);  // 输出模式
        write_reg8(REG_PB_OEN, pb_oen);
        
        uint8_t pb_input = read_reg8(REG_PB_INPUT);
        pb_input &= ~(1 << pin);
        write_reg8(REG_PB_INPUT, pb_input);
    }
    
    memset(&adc_status, 0, sizeof(adc_status));
    memset(adc_config, 0, sizeof(adc_config));
    scan_mode = false;
}

bool adc_config_channel(uint8_t ch, const ADC_ChannelConfigTypeDef *config)
{
    if (ch >= ADC_CHANNEL_COUNT || config == NULL) {
        return false;
    }
    
    // 验证引脚
    if (config->pin < 8 || config->pin > 13) {  // 必须是PB0-PB5
        return false;
    }
    
    // 保存配置
    memcpy(&adc_config[ch], config, sizeof(ADC_ChannelConfigTypeDef));
    
    // 配置GPIO
    adc_gpio_init(ch);
    
    return true;
}

uint16_t adc_read_raw(uint8_t ch)
{
    if (ch >= ADC_CHANNEL_COUNT || !adc_config[ch].enabled) {
        return 0;
    }
    
    uint8_t adc_ch = adc_ch_map[ch];
    
    adc_status.sample_count++;
    adc_status.last_sample_ms = 0;  // TODO: 需要系统时间
    
    return adc_oversample_read(adc_ch, adc_config[ch].oversample);
}

float adc_read_voltage(uint8_t ch)
{
    uint16_t raw = adc_read_raw(ch);
    
    if (raw == 0) {
        return 0.0f;
    }
    
    float voltage = adc_raw_to_voltage(raw);
    
    // 应用校准
    voltage += adc_config[ch].offset_mv;
    
    return voltage;
}

ADC_SampleTypeDef adc_sample(uint8_t ch)
{
    ADC_SampleTypeDef result = {0};
    
    if (ch >= ADC_CHANNEL_COUNT || !adc_config[ch].enabled) {
        result.valid = false;
        return result;
    }
    
    // 读取原始值
    result.raw = adc_read_raw(ch);
    result.voltage_mv = adc_read_voltage(ch);
    result.timestamp_ms = 0;  // TODO: 系统时间
    result.valid = (result.raw > 0);
    
    // 根据功能计算物理量
    switch (adc_config[ch].function) {
        case ADC_FUNC_IMPEDANCE:
            // 分压法：Vout = Vref * Z / (R + Z)
            // Z = R * Vout / (Vref - Vout)
            if (result.voltage_mv > 0 && result.voltage_mv < ADC_VREF_MV) {
                float ratio = result.voltage_mv / (ADC_VREF_MV - result.voltage_mv);
                result.value = adc_config[ch].scale_factor * ratio;
            } else {
                result.value = 0;
                result.valid = false;
            }
            break;
            
        case ADC_FUNC_CURRENT:
            // I = V / (R * Gain)
            result.value = result.voltage_mv / adc_config[ch].scale_factor;
            break;
            
        case ADC_FUNC_VOLTAGE:
            // 分压后的电压需要还原
            result.value = result.voltage_mv * adc_config[ch].scale_factor;
            break;
            
        case ADC_FUNC_POTENTIOMETER:
            // 直接映射到0-10000
            result.value = (float)result.raw * adc_config[ch].scale_factor;
            if (result.value > 10000.0f) result.value = 10000.0f;
            break;
            
        case ADC_FUNC_TEMPERATURE:
            // 简化线性模型（实际需要查表或校准）
            result.value = (result.voltage_mv - 500.0f) / 10.0f;  // 假设10mV/°C
            break;
            
        default:
            result.value = result.voltage_mv;
            break;
    }
    
    // 保存到缓冲区
    adc_sample_buf[ch] = result;
    
    // 回调
    if (adc_callback) {
        adc_callback(ch, result);
    }
    
    return result;
}

uint8_t adc_sample_multi(const uint8_t *channels, uint8_t count, ADC_SampleTypeDef *results)
{
    if (channels == NULL || results == NULL || count == 0) {
        return 0;
    }
    
    uint8_t success = 0;
    
    for (uint8_t i = 0; i < count; i++) {
        uint8_t ch = channels[i];
        if (ch < ADC_CHANNEL_COUNT) {
            results[i] = adc_sample(ch);
            if (results[i].valid) {
                success++;
            }
        }
    }
    
    return success;
}

uint16_t adc_read_impedance(uint8_t ch)
{
    if (ch >= ADC_CHANNEL_COUNT) {
        return 0;
    }
    
    // 临时切换为阻抗检测功能
    ADC_FunctionTypeDef old_func = adc_config[ch].function;
    adc_config[ch].function = ADC_FUNC_IMPEDANCE;
    
    ADC_SampleTypeDef sample = adc_sample(ch);
    
    // 恢复功能
    adc_config[ch].function = old_func;
    
    if (sample.valid && sample.value > 0) {
        return (uint16_t)sample.value;
    }
    
    return 0;
}

float adc_read_current(uint8_t ch)
{
    if (ch >= ADC_CHANNEL_COUNT) {
        return -1.0f;
    }
    
    // 临时切换为电流检测功能
    ADC_FunctionTypeDef old_func = adc_config[ch].function;
    adc_config[ch].function = ADC_FUNC_CURRENT;
    
    ADC_SampleTypeDef sample = adc_sample(ch);
    
    // 恢复功能
    adc_config[ch].function = old_func;
    
    if (sample.valid) {
        // 安全检查
        adc_safety_check(ch, sample.value, 0);
        
        return sample.value;
    }
    
    return -1.0f;
}

float adc_read_output_voltage(uint8_t ch)
{
    if (ch >= ADC_CHANNEL_COUNT) {
        return -1.0f;
    }
    
    // 临时切换为电压检测功能
    ADC_FunctionTypeDef old_func = adc_config[ch].function;
    adc_config[ch].function = ADC_FUNC_VOLTAGE;
    
    ADC_SampleTypeDef sample = adc_sample(ch);
    
    // 恢复功能
    adc_config[ch].function = old_func;
    
    if (sample.valid) {
        return sample.value / 1000.0f;  // mV → V
    }
    
    return -1.0f;
}

uint16_t adc_read_potentiometer(uint8_t ch)
{
    if (ch >= ADC_CHANNEL_COUNT) {
        return 0;
    }
    
    // 临时切换为电位器功能
    ADC_FunctionTypeDef old_func = adc_config[ch].function;
    adc_config[ch].function = ADC_FUNC_POTENTIOMETER;
    
    ADC_SampleTypeDef sample = adc_sample(ch);
    
    // 恢复功能
    adc_config[ch].function = old_func;
    
    if (sample.valid) {
        return (uint16_t)sample.value;
    }
    
    return 0;
}

bool adc_check_electrode(uint8_t ch)
{
    uint16_t impedance = adc_read_impedance(ch);
    
    return (impedance >= ADC_IMPEDANCE_MIN_OHM && 
            impedance <= ADC_IMPEDANCE_MAX_OHM);
}

bool adc_calibrate(void)
{
    // 零点校准：所有通道接地，测量偏移
    // 注意：实际校准时需要外部短接输入到地
    
    for (uint8_t ch = 0; ch < ADC_CHANNEL_COUNT; ch++) {
        if (!adc_config[ch].enabled) continue;
        
        // 采样多次取平均
        uint32_t sum = 0;
        for (uint8_t i = 0; i < 32; i++) {
            sum += adc_convert_single(adc_ch_map[ch]);
            adc_delay_ms(1);
        }
        
        uint16_t avg = (uint16_t)(sum / 32);
        float offset = adc_raw_to_voltage(avg);
        
        adc_config[ch].offset_mv = -offset;  // 补偿偏移
    }
    
    adc_status.calibration_done = true;
    
    return true;
}

void adc_set_calibration(uint8_t ch, float scale, float offset_mv)
{
    if (ch >= ADC_CHANNEL_COUNT) {
        return;
    }
    
    adc_config[ch].scale_factor = scale;
    adc_config[ch].offset_mv = offset_mv;
}

bool adc_start_scan(const uint8_t *channels, uint8_t count, uint16_t interval_ms)
{
    if (channels == NULL || count == 0 || count > ADC_CHANNEL_COUNT) {
        return false;
    }
    
    // 保存扫描配置
    scan_count = count;
    scan_interval_ms = interval_ms;
    last_scan_ms = 0;
    
    for (uint8_t i = 0; i < count; i++) {
        scan_channels[i] = channels[i];
    }
    
    scan_mode = true;
    
    return true;
}

void adc_stop_scan(void)
{
    scan_mode = false;
    scan_count = 0;
}

ADC_SampleTypeDef adc_get_scan_result(uint8_t ch)
{
    if (ch >= ADC_CHANNEL_COUNT) {
        ADC_SampleTypeDef empty = {0};
        return empty;
    }
    
    return adc_sample_buf[ch];
}

const ADC_StatusTypeDef* adc_get_status(void)
{
    return &adc_status;
}

void adc_set_callback(void (*callback)(uint8_t ch, ADC_SampleTypeDef sample))
{
    adc_callback = callback;
}

void adc_set_safety_callback(void (*callback)(uint8_t ch, uint8_t type, float value))
{
    safety_callback = callback;
}

/* ============================================================
 * 扫描处理（由主循环或定时器调用）
 * ============================================================ */
void adc_scan_handler(void)
{
    if (!scan_mode || scan_count == 0) {
        return;
    }
    
    // TODO: 需要系统时间判断间隔
    // 简化实现：每次调用扫描一个通道
    static uint8_t current_idx = 0;
    
    uint8_t ch = scan_channels[current_idx];
    if (ch < ADC_CHANNEL_COUNT) {
        adc_sample(ch);
    }
    
    current_idx++;
    if (current_idx >= scan_count) {
        current_idx = 0;
    }
}
