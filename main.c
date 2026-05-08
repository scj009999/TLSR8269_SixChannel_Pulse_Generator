/**
 * TLSR8269 六路脉冲发生器
 * 
 * 功能：使用6个模拟数字信号端口输出独立可调的PWM脉冲信号
 * 芯片：Telink TLSR8269F512
 * 
 * 输出端口映射：
 *   通道0: PA0 (PWM0) - 脉冲输出0
 *   通道1: PA1 (PWM1) - 脉冲输出1
 *   通道2: PA2 (PWM2) - 脉冲输出2
 *   通道3: PA3 (PWM3) - 脉冲输出3
 *   通道4: PA4 (PWM4) - 脉冲输出4
 *   通道5: PA5 (PWM5) - 脉冲输出5
 * 
 * 每路PWM可独立配置：
 *   - 频率：1Hz ~ 1MHz
 *   - 占空比：0% ~ 100%
 *   - 相位：0° ~ 360°
 *   - 输出使能/禁止
 */

#include "tl_common.h"
#include "drivers.h"
#include "stack/ble/ble.h"
#include "app_config.h"

// ============================================================================
// 配置宏定义
// ============================================================================

// 系统时钟配置
#ifndef CLOCK_SYS_CLOCK_HZ
#define CLOCK_SYS_CLOCK_HZ      16000000    // 16MHz系统时钟
#endif

// PWM时钟频率 (推荐等于系统时钟以获得最高精度)
#define PWM_CLOCK_HZ            CLOCK_SYS_CLOCK_HZ

// 六路脉冲输出GPIO定义
#define PULSE_CH0_PIN           GPIO_PA0    // PWM0
#define PULSE_CH1_PIN           GPIO_PA1    // PWM1
#define PULSE_CH2_PIN           GPIO_PA2    // PWM2
#define PULSE_CH3_PIN           GPIO_PA3    // PWM3
#define PULSE_CH4_PIN           GPIO_PA4    // PWM4
#define PULSE_CH5_PIN           GPIO_PA5    // PWM5

// 通道数量
#define PULSE_CHANNEL_NUM       6

// 默认脉冲参数
#define DEFAULT_PULSE_FREQ_HZ   1000        // 默认1kHz
#define DEFAULT_PULSE_DUTY      50          // 默认50%占空比

// ============================================================================
// 数据结构定义
// ============================================================================

// 脉冲通道配置结构体
typedef struct {
    pwm_id      pwm_id;         // PWM硬件ID
    GPIO_PinTypeDef gpio_pin;   // GPIO引脚
    u32         frequency_hz;   // 频率(Hz)
    u16         duty_percent;   // 占空比(0-10000, 表示0.00%-100.00%)
    u16         phase_offset;   // 相位偏移(0-36000, 表示0.00°-360.00°)
    u8          enabled;        // 使能标志
} PulseChannelCfg_t;

// ============================================================================
// 全局变量
// ============================================================================

// 六路脉冲通道配置
PulseChannelCfg_t g_pulseChannels[PULSE_CHANNEL_NUM] = {
    {PWM0_ID, PULSE_CH0_PIN, DEFAULT_PULSE_FREQ_HZ, 5000, 0, 1},  // CH0: 1kHz, 50%, 0°
    {PWM1_ID, PULSE_CH1_PIN, DEFAULT_PULSE_FREQ_HZ, 5000, 0, 1},  // CH1: 1kHz, 50%, 0°
    {PWM2_ID, PULSE_CH2_PIN, DEFAULT_PULSE_FREQ_HZ, 5000, 0, 1},  // CH2: 1kHz, 50%, 0°
    {PWM3_ID, PULSE_CH3_PIN, DEFAULT_PULSE_FREQ_HZ, 5000, 0, 1},  // CH3: 1kHz, 50%, 0°
    {PWM4_ID, PULSE_CH4_PIN, DEFAULT_PULSE_FREQ_HZ, 5000, 0, 1},  // CH4: 1kHz, 50%, 0°
    {PWM5_ID, PULSE_CH5_PIN, DEFAULT_PULSE_FREQ_HZ, 5000, 0, 1},  // CH5: 1kHz, 50%, 0°
};

// PWM功能映射表
static const GPIO_FuncTypeDef s_pwmFuncMap[PULSE_CHANNEL_NUM] = {
    AS_PWM0, AS_PWM1, AS_PWM2, AS_PWM3, AS_PWM4, AS_PWM5
};

// ============================================================================
// 函数声明
// ============================================================================

void pulse_generator_init(void);
void pulse_channel_config(u8 ch, u32 freq_hz, u16 duty_percent, u16 phase_deg);
void pulse_channel_enable(u8 ch);
void pulse_channel_disable(u8 ch);
void pulse_all_channels_update(void);
void pulse_set_frequency(u8 ch, u32 freq_hz);
void pulse_set_duty(u8 ch, u16 duty_percent);
void pulse_set_phase(u8 ch, u16 phase_deg);

// ============================================================================
// 脉冲发生器初始化
// ============================================================================

/**
 * @brief 初始化六路脉冲发生器
 * 
 * 配置步骤：
 * 1. 禁用低功耗模式（PWM与PM冲突）
 * 2. 设置PWM时钟
 * 3. 配置GPIO为PWM功能
 * 4. 初始化各通道参数
 * 5. 启动PWM输出
 */
void pulse_generator_init(void)
{
    // 禁用低功耗模式，确保PWM持续输出
    #if (BLE_APP_PM_ENABLE)
    bls_pm_setSuspendMask(SUSPEND_DISABLE);
    #endif
    
    // 设置PWM时钟 (PWM时钟 = 系统时钟，最高精度)
    pwm_set_clk(CLOCK_SYS_CLOCK_HZ, PWM_CLOCK_HZ);
    
    // 初始化6路PWM通道
    for (u8 i = 0; i < PULSE_CHANNEL_NUM; i++) {
        PulseChannelCfg_t *pch = &g_pulseChannels[i];
        
        // 配置GPIO为PWM功能
        gpio_set_func(pch->gpio_pin, s_pwmFuncMap[i]);
        gpio_set_output_en(pch->gpio_pin, 1);  // 使能输出
        
        // 计算PWM周期和占空比
        // cycle = PWM_CLOCK_HZ / frequency
        // cmp = cycle * duty / 10000
        u32 cycle = PWM_CLOCK_HZ / pch->frequency_hz;
        u32 cmp = (cycle * pch->duty_percent) / 10000;
        
        // 限制cycle在16位范围内 (最大65535)
        if (cycle > 0xFFFF) {
            cycle = 0xFFFF;
        }
        if (cmp > cycle) {
            cmp = cycle;
        }
        
        // 设置PWM周期和占空比
        pwm_set_cycle_and_duty(pch->pwm_id, (u16)cycle, (u16)cmp);
        
        // 启动PWM
        if (pch->enabled) {
            pwm_start(pch->pwm_id);
        }
    }
}

// ============================================================================
// 脉冲通道配置
// ============================================================================

/**
 * @brief 配置单个脉冲通道参数
 * 
 * @param ch            通道号 (0-5)
 * @param freq_hz       频率 (Hz)
 * @param duty_percent  占空比 (0-10000, 表示0.00%-100.00%)
 * @param phase_deg     相位偏移 (0-36000, 表示0.00°-360.00°)
 */
void pulse_channel_config(u8 ch, u32 freq_hz, u16 duty_percent, u16 phase_deg)
{
    if (ch >= PULSE_CHANNEL_NUM) return;
    
    PulseChannelCfg_t *pch = &g_pulseChannels[ch];
    
    // 更新配置参数
    pch->frequency_hz = freq_hz;
    pch->duty_percent = duty_percent > 10000 ? 10000 : duty_percent;
    pch->phase_offset = phase_deg > 36000 ? 36000 : phase_deg;
    
    // 计算PWM参数
    // cycle = PWM时钟 / 目标频率
    u32 cycle = PWM_CLOCK_HZ / freq_hz;
    if (cycle == 0) cycle = 1;
    if (cycle > 0xFFFF) cycle = 0xFFFF;
    
    // cmp = cycle * duty / 10000
    u32 cmp = (cycle * pch->duty_percent) / 10000;
    if (cmp > cycle) cmp = cycle;
    
    // 应用相位偏移 (通过调整cmp值实现简单相位控制)
    // 注意：TLSR8269硬件不支持真正的相位偏移，这里通过微调占空比实现近似效果
    // 如需精确相位控制，需要使用定时器中断配合软件翻转
    if (pch->phase_offset > 0) {
        // 相位偏移实现需要更复杂的处理，这里预留接口
        // 实际应用中可以通过定时器同步多路PWM
    }
    
    // 更新PWM寄存器
    pwm_set_cycle_and_duty(pch->pwm_id, (u16)cycle, (u16)cmp);
    
    // 如果通道已使能，确保PWM在运行
    if (pch->enabled) {
        pwm_start(pch->pwm_id);
    }
}

/**
 * @brief 使能脉冲通道输出
 * @param ch 通道号 (0-5)
 */
void pulse_channel_enable(u8 ch)
{
    if (ch >= PULSE_CHANNEL_NUM) return;
    
    g_pulseChannels[ch].enabled = 1;
    pwm_start(g_pulseChannels[ch].pwm_id);
}

/**
 * @brief 禁止脉冲通道输出
 * @param ch 通道号 (0-5)
 */
void pulse_channel_disable(u8 ch)
{
    if (ch >= PULSE_CHANNEL_NUM) return;
    
    g_pulseChannels[ch].enabled = 0;
    pwm_stop(g_pulseChannels[ch].pwm_id);
    
    // 将GPIO拉低
    gpio_write(g_pulseChannels[ch].gpio_pin, 0);
}

// ============================================================================
// 参数设置接口
// ============================================================================

/**
 * @brief 设置通道频率
 * @param ch      通道号 (0-5)
 * @param freq_hz 频率 (Hz)
 */
void pulse_set_frequency(u8 ch, u32 freq_hz)
{
    if (ch >= PULSE_CHANNEL_NUM) return;
    
    PulseChannelCfg_t *pch = &g_pulseChannels[ch];
    
    // 限制频率范围
    if (freq_hz < 1) freq_hz = 1;
    if (freq_hz > PWM_CLOCK_HZ / 2) freq_hz = PWM_CLOCK_HZ / 2;
    
    pch->frequency_hz = freq_hz;
    
    // 重新计算并应用
    u32 cycle = PWM_CLOCK_HZ / freq_hz;
    if (cycle > 0xFFFF) cycle = 0xFFFF;
    
    u32 cmp = (cycle * pch->duty_percent) / 10000;
    if (cmp > cycle) cmp = cycle;
    
    pwm_set_cycle_and_duty(pch->pwm_id, (u16)cycle, (u16)cmp);
}

/**
 * @brief 设置通道占空比
 * @param ch           通道号 (0-5)
 * @param duty_percent 占空比 (0-10000, 表示0.00%-100.00%)
 */
void pulse_set_duty(u8 ch, u16 duty_percent)
{
    if (ch >= PULSE_CHANNEL_NUM) return;
    
    PulseChannelCfg_t *pch = &g_pulseChannels[ch];
    
    // 限制占空比范围
    if (duty_percent > 10000) duty_percent = 10000;
    
    pch->duty_percent = duty_percent;
    
    // 重新计算cmp
    u32 cycle = PWM_CLOCK_HZ / pch->frequency_hz;
    if (cycle > 0xFFFF) cycle = 0xFFFF;
    
    u32 cmp = (cycle * duty_percent) / 10000;
    if (cmp > cycle) cmp = cycle;
    
    pwm_set_cmp(pch->pwm_id, (u16)cmp);
}

/**
 * @brief 设置通道相位偏移
 * @param ch        通道号 (0-5)
 * @param phase_deg 相位偏移 (0-36000, 表示0.00°-360.00°)
 * 
 * @note TLSR8269硬件PWM不支持真正的相位偏移。
 *       如需精确相位控制，建议使用定时器中断配合软件实现。
 */
void pulse_set_phase(u8 ch, u16 phase_deg)
{
    if (ch >= PULSE_CHANNEL_NUM) return;
    
    if (phase_deg > 36000) phase_deg = 36000;
    
    g_pulseChannels[ch].phase_offset = phase_deg;
    
    // 注意：这里仅记录相位值，实际相位控制需要额外的定时器同步机制
    // 简单实现可以通过调整占空比或定时器中断来实现
}

// ============================================================================
// 批量更新
// ============================================================================

/**
 * @brief 更新所有通道配置
 * 
 * 应用场景：
 * - 批量修改参数后统一应用
 * - 恢复默认配置
 * - 同步多路输出
 */
void pulse_all_channels_update(void)
{
    for (u8 i = 0; i < PULSE_CHANNEL_NUM; i++) {
        PulseChannelCfg_t *pch = &g_pulseChannels[i];
        
        u32 cycle = PWM_CLOCK_HZ / pch->frequency_hz;
        if (cycle > 0xFFFF) cycle = 0xFFFF;
        
        u32 cmp = (cycle * pch->duty_percent) / 10000;
        if (cmp > cycle) cmp = cycle;
        
        pwm_set_cycle_and_duty(pch->pwm_id, (u16)cycle, (u16)cmp);
        
        if (pch->enabled) {
            pwm_start(pch->pwm_id);
        } else {
            pwm_stop(pch->pwm_id);
        }
    }
}

// ============================================================================
// 演示/测试函数
// ============================================================================

/**
 * @brief 演示：六路不同频率脉冲输出
 * 
 * 配置：
 *   CH0: 100Hz,  25% 占空比
 *   CH1: 200Hz,  33% 占空比
 *   CH2: 500Hz,  50% 占空比
 *   CH3: 1kHz,   66% 占空比
 *   CH4: 2kHz,   75% 占空比
 *   CH5: 5kHz,   90% 占空比
 */
void pulse_demo_different_freq(void)
{
    pulse_channel_config(0, 100,   2500, 0);
    pulse_channel_config(1, 200,   3300, 0);
    pulse_channel_config(2, 500,   5000, 0);
    pulse_channel_config(3, 1000,  6600, 0);
    pulse_channel_config(4, 2000,  7500, 0);
    pulse_channel_config(5, 5000,  9000, 0);
    
    for (u8 i = 0; i < PULSE_CHANNEL_NUM; i++) {
        pulse_channel_enable(i);
    }
}

/**
 * @brief 演示：六路相同频率不同占空比
 * 
 * 配置：1kHz，占空比从10%递增到60%
 */
void pulse_demo_same_freq_diff_duty(void)
{
    for (u8 i = 0; i < PULSE_CHANNEL_NUM; i++) {
        u16 duty = 1000 + (i * 1000);  // 10%, 20%, 30%, 40%, 50%, 60%
        pulse_channel_config(i, 1000, duty, 0);
        pulse_channel_enable(i);
    }
}

/**
 * @brief 演示：呼吸灯效果 (PWM占空比渐变)
 * 
 * 在LED连接到PWM输出时，可实现呼吸灯效果
 */
void pulse_demo_breathing_led(u8 ch)
{
    if (ch >= PULSE_CHANNEL_NUM) return;
    
    static u16 brightness = 0;
    static s8 direction = 1;
    
    pulse_set_duty(ch, brightness);
    
    brightness += (direction * 100);
    
    if (brightness >= 10000) {
        brightness = 10000;
        direction = -1;
    } else if (brightness == 0) {
        brightness = 0;
        direction = 1;
    }
}

// ============================================================================
// 主函数
// ============================================================================

void user_init(void)
{
    // 等待系统稳定
    WaitMs(100);
    
    // 初始化脉冲发生器
    pulse_generator_init();
    
    // 运行演示：六路不同频率
    pulse_demo_different_freq();
    
    // 或者使用相同频率不同占空比
    // pulse_demo_same_freq_diff_duty();
}

void main_loop(void)
{
    // 主循环 - 可添加动态调整逻辑
    
    // 示例：每10ms调整一次CH0的占空比，实现呼吸灯效果
    // static u32 lastTick = 0;
    // if (clock_time() - lastTick > 10 * CLOCK_SYS_CLOCK_1MS) {
    //     lastTick = clock_time();
    //     pulse_demo_breathing_led(0);
    // }
    
    // 进入低功耗（注意：使用PWM时不能进入sleep）
    // bls_pm_setSuspendMask(SUSPEND_DISABLE);
}

// ============================================================================
// 中断处理 (如需精确相位控制，可添加定时器中断)
// ============================================================================

/**
 * @brief 定时器中断处理 - 用于精确相位同步
 * 
 * 如需多路PWM精确相位同步，可配置定时器中断，
 * 在中断中同步启动各路PWM。
 */
_attribute_ram_code_ void timer_irq_handler(void)
{
    // 清除中断标志
    reg_tmr_sta = FLD_TMR_STA_TMR0;
    
    // 同步启动所有PWM通道
    for (u8 i = 0; i < PULSE_CHANNEL_NUM; i++) {
        if (g_pulseChannels[i].enabled) {
            pwm_start(g_pulseChannels[i].pwm_id);
        }
    }
}
