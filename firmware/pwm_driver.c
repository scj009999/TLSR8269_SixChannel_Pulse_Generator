/**
 * @file pwm_driver.c
 * @brief TLSR8269 六通道PWM驱动实现
 * @details 基于Telink SDK PWM硬件模块，支持6路独立输出
 * @author 束长江
 * @version 1.0.0
 * @date 2026-05-24
 */

#include "pwm_driver.h"
#include "tlsr8269_reg.h"
#include <string.h>

/* ============================================================
 * TLSR8269 PWM寄存器定义
 * ============================================================ */
#define PWM_BASE_ADDR       0x800140  // PWM模块基地址

// PWM通道寄存器偏移
#define PWM_CH0_OFFSET      0x00
#define PWM_CH1_OFFSET      0x10
#define PWM_CH2_OFFSET      0x20
#define PWM_CH3_OFFSET      0x30
#define PWM_CH4_OFFSET      0x40
#define PWM_CH5_OFFSET      0x50

// 寄存器定义（每个通道）
#define REG_PWM_CYCLE(ch)   (PWM_BASE_ADDR + (ch) * 0x10 + 0x00)  // 周期
#define REG_PWM_DUTY(ch)    (PWM_BASE_ADDR + (ch) * 0x10 + 0x02)  // 占空比
#define REG_PWM_CTRL(ch)    (PWM_BASE_ADDR + (ch) * 0x10 + 0x04)  // 控制

// 全局控制
#define REG_PWM_EN          (PWM_BASE_ADDR + 0x60)  // 使能寄存器
#define REG_PWM_CLK         (PWM_BASE_ADDR + 0x61)  // 时钟配置
#define REG_PWM_INT         (PWM_BASE_ADDR + 0x62)  // 中断控制
#define REG_PWM_INT_MASK    (PWM_BASE_ADDR + 0x63)  // 中断屏蔽

// 控制位
#define PWM_CTRL_EN         (1 << 0)    // 通道使能
#define PWM_CTRL_POL        (1 << 1)    // 极性
#define PWM_CTRL_INT_EN     (1 << 2)    // 中断使能
#define PWM_CTRL_DMA_EN     (1 << 3)    // DMA使能

// 全局使能位
#define PWM_EN_CH0          (1 << 0)
#define PWM_EN_CH1          (1 << 1)
#define PWM_EN_CH2          (1 << 2)
#define PWM_EN_CH3          (1 << 3)
#define PWM_EN_CH4          (1 << 4)
#define PWM_EN_CH5          (1 << 5)
#define PWM_EN_ALL          0x3F

/* ============================================================
 * GPIO配置（PWM引脚复用）
 * ============================================================ */
#define GPIO_BASE_ADDR      0x800580

// PA功能选择：0=GPIO, 1=PWM
#define REG_PA_FUNC         (GPIO_BASE_ADDR + 0x48)
#define PA_FUNC_PWM0_3      ((1 << 0) | (1 << 1) | (1 << 2) | (1 << 3))

// PB功能选择
#define REG_PB_FUNC         (GPIO_BASE_ADDR + 0x4B)
#define PB_FUNC_PWM4_5      ((1 << 0) | (1 << 1))

// PA输出使能
#define REG_PA_OUTPUT       (GPIO_BASE_ADDR + 0x49)
#define REG_PA_OEN          (GPIO_BASE_ADDR + 0x4A)

// PB输出使能
#define REG_PB_OUTPUT       (GPIO_BASE_ADDR + 0x4C)
#define REG_PB_OEN          (GPIO_BASE_ADDR + 0x4F)

/* ============================================================
 * 时钟配置
 * ============================================================ */
// 系统时钟48MHz，PWM时钟 = 系统时钟 / (clk_div + 1)
// 目标：最大支持8MHz，最小1Hz
// 使用16位计数器，最大周期65535

#define PWM_SYS_CLK         48000000UL  // 48MHz
#define PWM_CLK_DIV_MIN     0           // 48MHz / 1 = 48MHz
#define PWM_CLK_DIV_MAX     255         // 48MHz / 256 = 187.5kHz

/* ============================================================
 * 私有变量
 * ============================================================ */
static PWM_ChannelConfigTypeDef pwm_config[PWM_CHANNEL_COUNT];
static PWM_ChannelStatusTypeDef pwm_status[PWM_CHANNEL_COUNT];
static PWM_GlobalStatusTypeDef pwm_global = {0};

static void (*pulse_callback)(uint8_t ch, uint32_t count) = NULL;
static void (*fault_callback)(uint8_t ch, PWM_FaultTypeDef fault) = NULL;

static uint16_t current_limit_ma = PWM_CURRENT_LIMIT_MA;

/* ============================================================
 * 私有函数
 * ============================================================ */

/**
 * @brief 计算PWM时钟分频和周期值
 * @param freq_hz 目标频率
 * @param clk_div 输出分频值
 * @param period 输出周期值
 * @return true=成功
 */
static bool pwm_calc_params(float freq_hz, uint8_t *clk_div, uint16_t *period)
{
    if (freq_hz < PWM_FREQ_MIN || freq_hz > PWM_FREQ_MAX) {
        return false;
    }
    
    // 目标：找到合适的clk_div和period
    // freq = PWM_SYS_CLK / ((clk_div + 1) * (period + 1))
    // period = PWM_SYS_CLK / ((clk_div + 1) * freq) - 1
    
    // 优先使用较小的clk_div以获得更高分辨率
    for (uint8_t div = PWM_CLK_DIV_MIN; div <= PWM_CLK_DIV_MAX; div++) {
        float clk = (float)PWM_SYS_CLK / (div + 1);
        float p = clk / freq_hz - 1.0f;
        
        if (p >= 1.0f && p <= 65535.0f) {
            *clk_div = div;
            *period = (uint16_t)(p + 0.5f);
            return true;
        }
    }
    
    return false;
}

/**
 * @brief 配置GPIO为PWM功能
 */
static void pwm_gpio_init(uint8_t ch)
{
    switch (ch) {
        case 0: case 1: case 2: case 3:
            // PA0-PA3: 设置为PWM功能
            {
                uint8_t pa_func = read_reg8(REG_PA_FUNC);
                pa_func |= (1 << ch);
                write_reg8(REG_PA_FUNC, pa_func);
                
                // 输出使能
                uint8_t pa_oen = read_reg8(REG_PA_OEN);
                pa_oen |= (1 << ch);
                write_reg8(REG_PA_OEN, pa_oen);
            }
            break;
            
        case 4: case 5:
            // PB0-PB1: 设置为PWM功能
            {
                uint8_t pb_func = read_reg8(REG_PB_FUNC);
                pb_func |= (1 << (ch - 4));
                write_reg8(REG_PB_FUNC, pb_func);
                
                // 输出使能
                uint8_t pb_oen = read_reg8(REG_PB_OEN);
                pb_oen |= (1 << (ch - 4));
                write_reg8(REG_PB_OEN, pb_oen);
            }
            break;
    }
}

/**
 * @brief 恢复GPIO为普通输出
 */
static void pwm_gpio_deinit(uint8_t ch)
{
    switch (ch) {
        case 0: case 1: case 2: case 3:
            {
                uint8_t pa_func = read_reg8(REG_PA_FUNC);
                pa_func &= ~(1 << ch);
                write_reg8(REG_PA_FUNC, pa_func);
            }
            break;
            
        case 4: case 5:
            {
                uint8_t pb_func = read_reg8(REG_PB_FUNC);
                pb_func &= ~(1 << (ch - 4));
                write_reg8(REG_PB_FUNC, pb_func);
            }
            break;
    }
}

/**
 * @brief 更新通道状态
 */
static void pwm_update_status(uint8_t ch)
{
    // 这里应该读取ADC/INA219获取实际电流和阻抗
    // 简化实现：使用模拟值
    
    if (pwm_status[ch].state == PWM_STATE_RUNNING) {
        // 模拟电流检测（实际应由ADC驱动提供）
        pwm_status[ch].current_ma = 0.0f;  // 待ADC驱动实现
        pwm_status[ch].voltage_v = 0.0f;
        pwm_status[ch].impedance_ohm = 0;
    }
}

/**
 * @brief 安全检查
 */
static bool pwm_safety_check(uint8_t ch)
{
    // 检查急停
    if (pwm_global.emergency_stop) {
        pwm_status[ch].state = PWM_STATE_FAULT;
        pwm_status[ch].fault = PWM_FAULT_OVERCURRENT;  // 急停归类为过流保护
        return false;
    }
    
    // 检查电流（需要ADC驱动支持）
    // float current = pwm_read_current(ch);
    // if (current > current_limit_ma) { ... }
    
    return true;
}

/* ============================================================
 * API实现
 * ============================================================ */

bool pwm_init(void)
{
    if (pwm_global.initialized) {
        return true;
    }
    
    // 初始化全局状态
    memset(&pwm_global, 0, sizeof(pwm_global));
    memset(pwm_config, 0, sizeof(pwm_config));
    memset(pwm_status, 0, sizeof(pwm_status));
    
    pwm_global.initialized = true;
    pwm_global.emergency_stop = false;
    
    // 配置PWM时钟：默认48MHz不分频
    write_reg8(REG_PWM_CLK, 0);
    
    // 禁用所有通道
    write_reg8(REG_PWM_EN, 0);
    
    // 初始化所有通道为默认参数
    for (uint8_t ch = 0; ch < PWM_CHANNEL_COUNT; ch++) {
        pwm_config[ch].frequency_hz = PWM_FREQ_DEFAULT;
        pwm_config[ch].duty = PWM_DUTY_DEFAULT;
        pwm_config[ch].pulse_width_us = 0;
        pwm_config[ch].enabled = false;
        pwm_config[ch].inverted = false;
        
        pwm_status[ch].state = PWM_STATE_IDLE;
        pwm_status[ch].fault = PWM_FAULT_NONE;
        
        // 配置GPIO
        pwm_gpio_init(ch);
        
        // 设置默认周期和占空比
        uint8_t clk_div;
        uint16_t period;
        if (pwm_calc_params(PWM_FREQ_DEFAULT, &clk_div, &period)) {
            write_reg16(REG_PWM_CYCLE(ch), period);
            
            uint16_t duty_val = (period * PWM_DUTY_DEFAULT) / PWM_DUTY_MAX;
            write_reg16(REG_PWM_DUTY(ch), duty_val);
        }
        
        // 默认不使能
        write_reg8(REG_PWM_CTRL(ch), 0);
    }
    
    return true;
}

void pwm_deinit(void)
{
    if (!pwm_global.initialized) {
        return;
    }
    
    // 停止所有通道
    pwm_stop_all();
    
    // 禁用PWM模块
    write_reg8(REG_PWM_EN, 0);
    
    // 恢复GPIO
    for (uint8_t ch = 0; ch < PWM_CHANNEL_COUNT; ch++) {
        pwm_gpio_deinit(ch);
    }
    
    memset(&pwm_global, 0, sizeof(pwm_global));
    memset(pwm_config, 0, sizeof(pwm_config));
    memset(pwm_status, 0, sizeof(pwm_status));
}

bool pwm_config_channel(uint8_t ch, const PWM_ChannelConfigTypeDef *config)
{
    if (ch >= PWM_CHANNEL_COUNT || config == NULL) {
        return false;
    }
    
    // 参数检查
    if (config->frequency_hz < PWM_FREQ_MIN || config->frequency_hz > PWM_FREQ_MAX) {
        return false;
    }
    
    if (config->duty > PWM_DUTY_MAX) {
        return false;
    }
    
    // 保存配置
    memcpy(&pwm_config[ch], config, sizeof(PWM_ChannelConfigTypeDef));
    
    // 计算并设置周期
    uint8_t clk_div;
    uint16_t period;
    if (!pwm_calc_params(config->frequency_hz, &clk_div, &period)) {
        return false;
    }
    
    // 设置时钟（全局，所有通道共享）
    // 注意：实际应用中可能需要更复杂的时钟管理
    write_reg8(REG_PWM_CLK, clk_div);
    
    // 设置周期
    write_reg16(REG_PWM_CYCLE(ch), period);
    
    // 设置占空比
    uint16_t duty_val = (uint32_t)period * config->duty / PWM_DUTY_MAX;
    if (duty_val > period) duty_val = period;
    write_reg16(REG_PWM_DUTY(ch), duty_val);
    
    // 设置控制寄存器
    uint8_t ctrl = 0;
    if (config->inverted) {
        ctrl |= PWM_CTRL_POL;
    }
    write_reg8(REG_PWM_CTRL(ch), ctrl);
    
    // 更新状态
    pwm_status[ch].state = PWM_STATE_READY;
    pwm_status[ch].fault = PWM_FAULT_NONE;
    
    return true;
}

bool pwm_set_frequency(uint8_t ch, float freq_hz)
{
    if (ch >= PWM_CHANNEL_COUNT) {
        return false;
    }
    
    PWM_ChannelConfigTypeDef config = pwm_config[ch];
    config.frequency_hz = freq_hz;
    
    return pwm_config_channel(ch, &config);
}

bool pwm_set_duty(uint8_t ch, uint16_t duty)
{
    if (ch >= PWM_CHANNEL_COUNT || duty > PWM_DUTY_MAX) {
        return false;
    }
    
    // 更新占空比
    uint16_t period = read_reg16(REG_PWM_CYCLE(ch));
    uint16_t duty_val = (uint32_t)period * duty / PWM_DUTY_MAX;
    if (duty_val > period) duty_val = period;
    
    write_reg16(REG_PWM_DUTY(ch), duty_val);
    
    pwm_config[ch].duty = duty;
    
    return true;
}

bool pwm_start_channel(uint8_t ch)
{
    if (ch >= PWM_CHANNEL_COUNT) {
        return false;
    }
    
    // 安全检查
    if (!pwm_safety_check(ch)) {
        return false;
    }
    
    // 检查状态
    if (pwm_status[ch].state == PWM_STATE_FAULT) {
        return false;
    }
    
    // 使能通道
    uint8_t en = read_reg8(REG_PWM_EN);
    en |= (1 << ch);
    write_reg8(REG_PWM_EN, en);
    
    // 更新控制寄存器
    uint8_t ctrl = read_reg8(REG_PWM_CTRL(ch));
    ctrl |= PWM_CTRL_EN;
    write_reg8(REG_PWM_CTRL(ch), ctrl);
    
    // 更新状态
    pwm_status[ch].state = PWM_STATE_RUNNING;
    pwm_global.active_channels++;
    
    return true;
}

void pwm_stop_channel(uint8_t ch)
{
    if (ch >= PWM_CHANNEL_COUNT) {
        return;
    }
    
    // 禁用通道
    uint8_t en = read_reg8(REG_PWM_EN);
    en &= ~(1 << ch);
    write_reg8(REG_PWM_EN, en);
    
    uint8_t ctrl = read_reg8(REG_PWM_CTRL(ch));
    ctrl &= ~PWM_CTRL_EN;
    write_reg8(REG_PWM_CTRL(ch), ctrl);
    
    // 更新状态
    if (pwm_status[ch].state == PWM_STATE_RUNNING) {
        pwm_global.active_channels--;
    }
    pwm_status[ch].state = PWM_STATE_IDLE;
}

void pwm_start_all(void)
{
    for (uint8_t ch = 0; ch < PWM_CHANNEL_COUNT; ch++) {
        if (pwm_config[ch].enabled) {
            pwm_start_channel(ch);
        }
    }
}

void pwm_stop_all(void)
{
    // 禁用所有通道
    write_reg8(REG_PWM_EN, 0);
    
    for (uint8_t ch = 0; ch < PWM_CHANNEL_COUNT; ch++) {
        write_reg8(REG_PWM_CTRL(ch), 0);
        
        if (pwm_status[ch].state == PWM_STATE_RUNNING) {
            pwm_global.active_channels--;
        }
        pwm_status[ch].state = PWM_STATE_IDLE;
    }
}

void pwm_pause_all(void)
{
    // 保存当前使能状态并停止
    for (uint8_t ch = 0; ch < PWM_CHANNEL_COUNT; ch++) {
        if (pwm_status[ch].state == PWM_STATE_RUNNING) {
            pwm_status[ch].state = PWM_STATE_PAUSED;
        }
    }
    
    write_reg8(REG_PWM_EN, 0);
}

void pwm_resume_all(void)
{
    for (uint8_t ch = 0; ch < PWM_CHANNEL_COUNT; ch++) {
        if (pwm_status[ch].state == PWM_STATE_PAUSED) {
            pwm_start_channel(ch);
        }
    }
}

const PWM_ChannelStatusTypeDef* pwm_get_channel_status(uint8_t ch)
{
    if (ch >= PWM_CHANNEL_COUNT) {
        return NULL;
    }
    
    pwm_update_status(ch);
    return &pwm_status[ch];
}

const PWM_GlobalStatusTypeDef* pwm_get_global_status(void)
{
    return &pwm_global;
}

void pwm_emergency_stop(void)
{
    // 医疗级急停：<10ms响应
    // 直接写寄存器，不走正常流程
    
    // 立即禁用所有PWM输出
    write_reg8(REG_PWM_EN, 0);
    
    // 将所有控制寄存器清零
    for (uint8_t ch = 0; ch < PWM_CHANNEL_COUNT; ch++) {
        write_reg8(REG_PWM_CTRL(ch), 0);
        
        // GPIO设为低电平输出（安全状态）
        switch (ch) {
            case 0: case 1: case 2: case 3:
                {
                    uint8_t pa_out = read_reg8(REG_PA_OUTPUT);
                    pa_out &= ~(1 << ch);
                    write_reg8(REG_PA_OUTPUT, pa_out);
                }
                break;
            case 4: case 5:
                {
                    uint8_t pb_out = read_reg8(REG_PB_OUTPUT);
                    pb_out &= ~(1 << (ch - 4));
                    write_reg8(REG_PB_OUTPUT, pb_out);
                }
                break;
        }
    }
    
    // 更新状态
    pwm_global.emergency_stop = true;
    pwm_global.active_channels = 0;
    
    for (uint8_t ch = 0; ch < PWM_CHANNEL_COUNT; ch++) {
        if (pwm_status[ch].state == PWM_STATE_RUNNING ||
            pwm_status[ch].state == PWM_STATE_PAUSED) {
            pwm_status[ch].state = PWM_STATE_FAULT;
            pwm_status[ch].fault = PWM_FAULT_OVERCURRENT;
        }
    }
    
    // 调用故障回调
    if (fault_callback) {
        for (uint8_t ch = 0; ch < PWM_CHANNEL_COUNT; ch++) {
            fault_callback(ch, PWM_FAULT_OVERCURRENT);
        }
    }
}

void pwm_clear_emergency(void)
{
    pwm_global.emergency_stop = false;
    
    // 恢复GPIO为PWM功能
    for (uint8_t ch = 0; ch < PWM_CHANNEL_COUNT; ch++) {
        pwm_gpio_init(ch);
        pwm_status[ch].state = PWM_STATE_IDLE;
        pwm_status[ch].fault = PWM_FAULT_NONE;
    }
}

void pwm_clear_fault(uint8_t ch)
{
    if (ch >= PWM_CHANNEL_COUNT) {
        return;
    }
    
    pwm_status[ch].fault = PWM_FAULT_NONE;
    if (pwm_status[ch].state == PWM_STATE_FAULT) {
        pwm_status[ch].state = PWM_STATE_IDLE;
    }
}

void pwm_set_current_limit(uint16_t limit_ma)
{
    if (limit_ma > 0 && limit_ma <= 100) {  // 医疗级：最大100mA
        current_limit_ma = limit_ma;
    }
}

float pwm_read_current(uint8_t ch)
{
    if (ch >= PWM_CHANNEL_COUNT) {
        return -1.0f;
    }
    
    // TODO: 需要ADC驱动支持
    // 临时返回模拟值
    return pwm_status[ch].current_ma;
}

uint16_t pwm_read_impedance(uint8_t ch)
{
    if (ch >= PWM_CHANNEL_COUNT) {
        return 0;
    }
    
    // TODO: 需要ADC驱动支持
    return pwm_status[ch].impedance_ohm;
}

bool pwm_check_electrode(uint8_t ch)
{
    if (ch >= PWM_CHANNEL_COUNT) {
        return false;
    }
    
    uint16_t impedance = pwm_read_impedance(ch);
    
    // 阻抗在合理范围内表示接触良好
    return (impedance >= PWM_IMPEDANCE_MIN_OHM && 
            impedance <= PWM_IMPEDANCE_MAX_OHM);
}

void pwm_single_pulse(uint8_t ch, uint16_t pulse_width_us)
{
    if (ch >= PWM_CHANNEL_COUNT || pulse_width_us == 0) {
        return;
    }
    
    // 计算单脉冲参数
    // 频率 = 1 / (2 * pulse_width_us) * 1e6 Hz
    float freq = 1000000.0f / (2.0f * pulse_width_us);
    
    // 设置50%占空比
    pwm_set_frequency(ch, freq);
    pwm_set_duty(ch, 5000);  // 50%
    
    // 启动并延时一个周期后停止
    pwm_start_channel(ch);
    
    // 简单延时（实际应使用定时器）
    volatile uint32_t delay = pulse_width_us * 48;
    while (delay--) {
        __asm__("nop");
    }
    
    pwm_stop_channel(ch);
}

void pwm_set_pulse_callback(void (*callback)(uint8_t ch, uint32_t count))
{
    pulse_callback = callback;
}

void pwm_set_fault_callback(void (*callback)(uint8_t ch, PWM_FaultTypeDef fault))
{
    fault_callback = callback;
}

/* ============================================================
 * 中断处理（如果启用）
 * ============================================================ */
void pwm_irq_handler(void)
{
    uint8_t int_status = read_reg8(REG_PWM_INT);
    
    for (uint8_t ch = 0; ch < PWM_CHANNEL_COUNT; ch++) {
        if (int_status & (1 << ch)) {
            pwm_status[ch].pulse_count++;
            
            if (pulse_callback) {
                pulse_callback(ch, pwm_status[ch].pulse_count);
            }
        }
    }
    
    // 清除中断标志
    write_reg8(REG_PWM_INT, int_status);
}
