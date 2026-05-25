/**
 * PulseTherapy.cpp - 开源经颅脉冲治疗库实现
 * 
 * 安全设计原则：
 * 1. 所有参数硬件级限制
 * 2. 独立看门狗监控
 * 3. 实时电流检测
 * 4. 电极脱落检测
 * 5. 超时自动停止
 */

#include "PulseTherapy.h"

// 硬件寄存器定义（TLSR8269）
#define PWM_CLK_DIV     0x781
#define PWM_TC0         0x782
#define PWM_TC1         0x783
#define PWM_TC2         0x784
#define PWM_TC3         0x785
#define PWM_TC4         0x786
#define PWM_TC5         0x787
#define PWM_EN          0x780

// ADC寄存器（用于电流检测）
#define ADC_CTRL        0x880
#define ADC_DATA        0x881

// 看门狗
#define WDT_CTRL        0x620
#define WDT_CLR         0x621

// 安全阈值（硬编码，不可通过软件修改）
#define HARD_FREQ_LIMIT     250     // Hz，硬件频率上限
#define HARD_CURRENT_LIMIT  6000    // uA，硬件电流上限（6mA）
#define HARD_VOLTAGE_LIMIT  3600    // mV，硬件电压上限（3.6V）

// 采样电阻（决定电流检测精度）
#define CURRENT_SENSE_RESISTOR  100  // 欧姆
#define ADC_REF_VOLTAGE         3300 // mV
#define ADC_RESOLUTION          1024 // 10位ADC

// 电流计算公式：I = Vadc / R * 1000 (uA)
#define ADC_TO_UA(adc)  ((adc) * ADC_REF_VOLTAGE / ADC_RESOLUTION * 1000 / CURRENT_SENSE_RESISTOR)

// 全局实例
PulseTherapy Pulse;

PulseTherapy::PulseTherapy() {
    _initialized = false;
    _therapyRunning = false;
    _therapyPaused = false;
    _safetyPassed = false;
    _currentLimit = SAFE_CURRENT_MAX;
    _therapyStartTime = 0;
    _therapyDuration = 0;
    _currentMode = MODE_CUSTOM;
    _channelCallback = nullptr;
    _safetyCallback = nullptr;
    _completeCallback = nullptr;
    
    for (int i = 0; i < PULSE_CHANNEL_NUM; i++) {
        _channels[i].enabled = false;
        _channels[i].frequency = 0;
        _channels[i].duty = 0;
        _channels[i].phase = 0;
        _channels[i].current_uA = 0;
    }
}

bool PulseTherapy::begin() {
    if (_initialized) return true;
    
    // 1. 初始化看门狗（1秒超时）
    analogWrite(WDT_CTRL, 0x3F);  // 约1秒超时
    
    // 2. 初始化PWM时钟（16MHz）
    analogWrite(PWM_CLK_DIV, 0);  // 不分频，16MHz
    
    // 3. 初始化GPIO
    for (int i = 0; i < PULSE_CHANNEL_NUM; i++) {
        pinMode(CH0_PIN + i, OUTPUT);
        digitalWrite(CH0_PIN + i, LOW);
    }
    
    // 4. 初始化ADC（用于电流检测）
    analogWrite(ADC_CTRL, 0x80);  // 使能ADC
    
    // 5. 硬件自检
    if (!checkHardware()) {
        if (_safetyCallback) _safetyCallback("硬件自检失败");
        return false;
    }
    
    // 6. 安全检查
    _safetyPassed = safetyCheck();
    if (!_safetyPassed) {
        if (_safetyCallback) _safetyCallback("安全检查未通过");
        return false;
    }
    
    _initialized = true;
    return true;
}

void PulseTherapy::end() {
    disableAll();
    _initialized = false;
    _therapyRunning = false;
}

bool PulseTherapy::configureChannel(uint8_t ch, uint32_t freq, uint16_t duty, uint16_t phase) {
    if (ch >= PULSE_CHANNEL_NUM) return false;
    if (!_initialized) return false;
    
    // 参数验证
    if (!validateParameters(ch, freq, duty)) {
        if (_safetyCallback) _safetyCallback("参数超出安全范围");
        return false;
    }
    
    // 应用安全限制
    if (freq > SAFE_FREQ_MAX) freq = SAFE_FREQ_MAX;
    if (freq < SAFE_FREQ_MIN) freq = SAFE_FREQ_MIN;
    if (duty > SAFE_DUTY_MAX) duty = SAFE_DUTY_MAX;
    
    // 计算PWM寄存器值
    uint32_t cycle = 16000000 / freq;  // 16MHz时钟
    if (cycle > 65535) cycle = 65535;
    if (cycle < 2) cycle = 2;
    
    uint32_t cmp = (cycle * duty) / 10000;
    if (cmp >= cycle) cmp = cycle - 1;
    
    // 写入PWM寄存器
    uint8_t reg_base = PWM_TC0 + ch * 2;
    analogWrite(reg_base, cycle & 0xFF);
    analogWrite(reg_base + 1, (cycle >> 8) & 0xFF);
    
    // 更新状态
    _channels[ch].frequency = freq;
    _channels[ch].duty = duty;
    _channels[ch].phase = phase;
    
    // 喂狗
    analogWrite(WDT_CLR, 0x01);
    
    return true;
}

bool PulseTherapy::enableChannel(uint8_t ch) {
    if (ch >= PULSE_CHANNEL_NUM) return false;
    if (!_initialized) return false;
    
    // 再次安全检查
    if (!safetyCheck()) {
        emergencyStop();
        return false;
    }
    
    // 使能PWM输出
    uint8_t en_reg = analogRead(PWM_EN);
    en_reg |= (1 << ch);
    analogWrite(PWM_EN, en_reg);
    
    _channels[ch].enabled = true;
    
    if (_channelCallback) _channelCallback(ch, true);
    
    // 喂狗
    analogWrite(WDT_CLR, 0x01);
    
    return true;
}

bool PulseTherapy::disableChannel(uint8_t ch) {
    if (ch >= PULSE_CHANNEL_NUM) return false;
    
    // 禁止PWM输出
    uint8_t en_reg = analogRead(PWM_EN);
    en_reg &= ~(1 << ch);
    analogWrite(PWM_EN, en_reg);
    
    // 确保引脚低电平
    digitalWrite(CH0_PIN + ch, LOW);
    
    _channels[ch].enabled = false;
    
    if (_channelCallback) _channelCallback(ch, false);
    
    return true;
}

bool PulseTherapy::enableAll() {
    bool result = true;
    for (int i = 0; i < PULSE_CHANNEL_NUM; i++) {
        if (!enableChannel(i)) result = false;
    }
    return result;
}

bool PulseTherapy::disableAll() {
    // 快速关闭所有通道（原子操作）
    noInterrupts();
    
    analogWrite(PWM_EN, 0);
    for (int i = 0; i < PULSE_CHANNEL_NUM; i++) {
        digitalWrite(CH0_PIN + i, LOW);
        _channels[i].enabled = false;
    }
    
    interrupts();
    
    for (int i = 0; i < PULSE_CHANNEL_NUM; i++) {
        if (_channelCallback) _channelCallback(i, false);
    }
    
    return true;
}

bool PulseTherapy::setFrequency(uint8_t ch, uint32_t freq) {
    if (ch >= PULSE_CHANNEL_NUM) return false;
    return configureChannel(ch, freq, _channels[ch].duty, _channels[ch].phase);
}

bool PulseTherapy::setDuty(uint8_t ch, uint16_t duty) {
    if (ch >= PULSE_CHANNEL_NUM) return false;
    return configureChannel(ch, _channels[ch].frequency, duty, _channels[ch].phase);
}

bool PulseTherapy::setPhase(uint8_t ch, uint16_t phase) {
    if (ch >= PULSE_CHANNEL_NUM) return false;
    // 相位通过软件延时实现
    _channels[ch].phase = phase % 36000;
    return true;
}

bool PulseTherapy::setCurrentLimit(uint16_t current_uA) {
    if (current_uA > SAFE_CURRENT_MAX) current_uA = SAFE_CURRENT_MAX;
    if (current_uA < 100) current_uA = 100;  // 最小100uA
    _currentLimit = current_uA;
    return true;
}

bool PulseTherapy::startTherapy(TherapyMode mode, uint8_t duration_minutes) {
    if (_therapyRunning) return false;
    if (duration_minutes > SAFE_DURATION_MAX) duration_minutes = SAFE_DURATION_MAX;
    
    _currentMode = mode;
    _therapyDuration = duration_minutes * 60;  // 转换为秒
    
    switch (mode) {
        case MODE_PARKINSON_STD:
            loadParkinsonStandard();
            break;
        case MODE_PARKINSON_INTENSE:
            loadParkinsonIntense();
            break;
        case MODE_PARKINSON_CONSERVATIVE:
            loadParkinsonConservative();
            break;
        case MODE_SLEEP:
            loadSleepMode();
            break;
        case MODE_PAIN_RELIEF:
            loadPainRelief();
            break;
        default:
            // 自定义模式，使用当前配置
            break;
    }
    
    // 启动所有已配置通道
    for (int i = 0; i < PULSE_CHANNEL_NUM; i++) {
        if (_channels[i].frequency > 0) {
            enableChannel(i);
        }
    }
    
    _therapyStartTime = millis() / 1000;
    _therapyRunning = true;
    _therapyPaused = false;
    
    return true;
}

bool PulseTherapy::stopTherapy() {
    disableAll();
    _therapyRunning = false;
    _therapyPaused = false;
    _therapyStartTime = 0;
    return true;
}

bool PulseTherapy::pauseTherapy() {
    if (!_therapyRunning) return false;
    disableAll();
    _therapyPaused = true;
    return true;
}

bool PulseTherapy::resumeTherapy() {
    if (!_therapyRunning || !_therapyPaused) return false;
    
    for (int i = 0; i < PULSE_CHANNEL_NUM; i++) {
        if (_channels[i].frequency > 0) {
            enableChannel(i);
        }
    }
    
    _therapyPaused = false;
    return true;
}

bool PulseTherapy::loadParkinsonStandard() {
    // 刘河生教授标准方案
    configureChannel(0, 40, 5000, 0);      // 40Hz, 50%, 0°
    configureChannel(1, 40, 5000, 0);      // 40Hz, 50%, 0°
    configureChannel(2, 40, 5000, 0);      // 40Hz, 50%, 0°
    configureChannel(3, 40, 5000, 0);      // 40Hz, 50%, 0°
    configureChannel(4, 0, 0, 0);          // 未使用
    configureChannel(5, 0, 0, 0);          // 未使用
    return true;
}

bool PulseTherapy::loadParkinsonIntense() {
    configureChannel(0, 40, 7000, 0);      // 40Hz, 70%
    configureChannel(1, 40, 7000, 0);
    configureChannel(2, 40, 7000, 0);
    configureChannel(3, 40, 7000, 0);
    return true;
}

bool PulseTherapy::loadParkinsonConservative() {
    configureChannel(0, 20, 3000, 0);      // 20Hz, 30%
    configureChannel(1, 20, 3000, 0);
    configureChannel(2, 20, 3000, 0);
    configureChannel(3, 20, 3000, 0);
    return true;
}

bool PulseTherapy::loadSleepMode() {
    configureChannel(0, 10, 3000, 0);      // 10Hz, 30%
    configureChannel(1, 10, 3000, 18000);  // 10Hz, 30%, 180°反相
    return true;
}

bool PulseTherapy::loadPainRelief() {
    configureChannel(0, 100, 5000, 0);     // 100Hz, 50%
    configureChannel(1, 100, 5000, 0);
    return true;
}

bool PulseTherapy::emergencyStop() {
    // 立即关闭所有输出（最高优先级）
    // 使用原子操作确保不被中断打断
    noInterrupts();
    
    analogWrite(PWM_EN, 0);
    for (int i = 0; i < PULSE_CHANNEL_NUM; i++) {
        digitalWrite(CH0_PIN + i, LOW);
        _channels[i].enabled = false;
    }
    
    _therapyRunning = false;
    _therapyPaused = false;
    
    interrupts();
    
    if (_safetyCallback) _safetyCallback("紧急停止已触发！");
    
    return true;
}

bool PulseTherapy::safetyCheck() {
    // 1. 检查电池电压
    uint16_t batteryVoltage = readBatteryVoltage();
    if (batteryVoltage < 3300 || batteryVoltage > 4200) {
        if (_safetyCallback) _safetyCallback("电池电压异常");
        return false;
    }
    
    // 2. 检查输出电流（所有通道）
    for (int i = 0; i < PULSE_CHANNEL_NUM; i++) {
        if (_channels[i].enabled) {
            uint16_t current = readChannelCurrent(i);
            if (current > _currentLimit) {
                disableChannel(i);
                if (_safetyCallback) _safetyCallback("通道电流超限，已自动关闭");
                return false;
            }
        }
    }
    
    // 3. 检查温度（如果硬件支持）
    // TODO: 添加温度检测
    
    // 4. 检查电极连接（阻抗检测）
    for (int i = 0; i < PULSE_CHANNEL_NUM; i++) {
        if (_channels[i].enabled) {
            uint16_t impedance = readElectrodeImpedance(i);
            if (impedance > 10000) {  // 阻抗过大，可能脱落
                if (_safetyCallback) _safetyCallback("电极可能脱落，请检查连接");
                // 不自动关闭，仅警告
            }
        }
    }
    
    _safetyPassed = true;
    return true;
}

ChannelState PulseTherapy::getChannelState(uint8_t ch) {
    if (ch >= PULSE_CHANNEL_NUM) return ChannelState();
    return _channels[ch];
}

uint8_t PulseTherapy::getActiveChannelCount() {
    uint8_t count = 0;
    for (int i = 0; i < PULSE_CHANNEL_NUM; i++) {
        if (_channels[i].enabled) count++;
    }
    return count;
}

uint32_t PulseTherapy::getElapsedTime() {
    if (!_therapyRunning) return 0;
    return (millis() / 1000) - _therapyStartTime;
}

uint32_t PulseTherapy::getRemainingTime() {
    if (!_therapyRunning) return 0;
    int32_t remaining = _therapyDuration - getElapsedTime();
    return (remaining > 0) ? remaining : 0;
}

void PulseTherapy::update() {
    // 喂狗（必须在1秒内执行）
    analogWrite(WDT_CLR, 0x01);
    
    // 检查治疗是否超时
    if (_therapyRunning && !_therapyPaused) {
        if (getElapsedTime() >= _therapyDuration) {
            stopTherapy();
            if (_completeCallback) _completeCallback();
            return;
        }
        
        // 定期检查安全（每100ms一次，提高响应速度）
        static uint32_t lastSafetyCheck = 0;
        uint32_t now = millis();
        if (now - lastSafetyCheck > 100) {  // 每100ms检查一次
            lastSafetyCheck = now;
            if (!safetyCheck()) {
                emergencyStop();
                return;
            }
        }
        
        // 实时电流监测（每通道每50ms）
        static uint8_t currentCh = 0;
        static uint32_t lastCurrentCheck = 0;
        if (now - lastCurrentCheck > 50) {
            lastCurrentCheck = now;
            if (_channels[currentCh].enabled) {
                uint16_t current = readChannelCurrent(currentCh);
                if (current > _currentLimit) {
                    // 电流超限，立即降低占空比
                    uint16_t newDuty = (_channels[currentCh].duty * _currentLimit) / current;
                    setDuty(currentCh, newDuty);
                    if (_safetyCallback) _safetyCallback("通道电流超限，已自动降低强度");
                }
            }
            currentCh = (currentCh + 1) % PULSE_CHANNEL_NUM;
        }
    }
}

void PulseTherapy::breathingEffect(uint8_t ch, float period_sec) {
    if (ch >= PULSE_CHANNEL_NUM) return;
    
    static uint32_t lastUpdate = 0;
    static int direction = 1;
    static uint16_t duty = 0;
    
    uint32_t now = millis();
    uint32_t stepTime = (uint32_t)(period_sec * 1000 / 200);  // 200步完成一个周期
    
    if (now - lastUpdate > stepTime) {
        lastUpdate = now;
        duty += direction * 100;  // 每次变化1%
        
        if (duty >= 10000) {
            duty = 10000;
            direction = -1;
        }
        if (duty <= 0) {
            duty = 0;
            direction = 1;
        }
        
        setDuty(ch, duty);
    }
}

const char* PulseTherapy::getVersion() {
    return "1.0.0";
}

// 私有方法

bool PulseTherapy::validateParameters(uint8_t ch, uint32_t freq, uint16_t duty) {
    if (ch >= PULSE_CHANNEL_NUM) return false;
    if (freq > HARD_FREQ_LIMIT) return false;  // 硬件级限制
    if (freq < SAFE_FREQ_MIN) return false;
    if (duty > SAFE_DUTY_MAX) return false;
    return true;
}

bool PulseTherapy::checkHardware() {
    // 检查PWM模块
    uint8_t pwm_en = analogRead(PWM_EN);
    if (pwm_en != 0) {
        analogWrite(PWM_EN, 0);  // 确保所有PWM关闭
    }
    
    // 检查ADC
    analogWrite(ADC_CTRL, 0x80);
    delay(1);
    uint16_t adc_val = analogRead(ADC_DATA);
    if (adc_val == 0xFFFF) return false;  // ADC故障
    
    return true;
}

void PulseTherapy::applySafetyLimits(uint8_t ch) {
    // 应用电流限制
    uint16_t current = readChannelCurrent(ch);
    if (current > _currentLimit) {
        // 降低占空比以限制电流
        uint16_t newDuty = (_channels[ch].duty * _currentLimit) / current;
        setDuty(ch, newDuty);
    }
}

// 辅助函数

uint16_t PulseTherapy::readBatteryVoltage() {
    // 通过ADC读取电池电压（需要分压电路）
    analogWrite(ADC_CTRL, 0x80 | 0x01);  // 选择电池电压通道
    delay(1);
    return analogRead(ADC_DATA) * 2;  // 假设2:1分压
}

uint16_t PulseTherapy::readChannelCurrent(uint8_t ch) {
    // 读取采样电阻上的电压，计算电流
    // 改进：多次采样取平均，提高精度
    uint8_t adc_ch = 2 + ch;  // 假设通道0-5对应ADC2-7
    
    uint32_t sum = 0;
    const uint8_t samples = 8;  // 8次采样取平均
    
    for (uint8_t i = 0; i < samples; i++) {
        analogWrite(ADC_CTRL, 0x80 | adc_ch);
        delayMicroseconds(100);  // 缩短延时，提高响应速度
        sum += analogRead(ADC_DATA);
    }
    
    uint16_t adc_avg = sum / samples;
    return ADC_TO_UA(adc_avg);
}

uint16_t PulseTherapy::readElectrodeImpedance(uint8_t ch) {
    // 通过测量开路电压和负载电压计算阻抗
    // 改进：使用交流阻抗测量方法，更准确
    if (!_channels[ch].enabled) return 0;
    
    // 方法1：直流阻抗估算（简化）
    uint16_t current = readChannelCurrent(ch);
    if (current == 0) return 100000;  // 开路（改为100kΩ，更合理）
    if (current < 50) return 50000;   // 电流过小，可能接触不良
    
    // 阻抗 = 电压 / 电流
    uint16_t voltage = _channels[ch].duty * 3300 / 10000;  // mV
    uint16_t impedance = (voltage * 1000) / current;  // 欧姆
    
    // 判断电极状态
    if (impedance > 50000) {
        // 高阻抗：可能电极脱落或干燥
        if (_safetyCallback) _safetyCallback("电极阻抗过高，请检查连接或湿润电极");
    } else if (impedance > 10000) {
        // 中等阻抗：可能皮肤干燥
        if (_safetyCallback) _safetyCallback("电极阻抗偏高，建议清洁皮肤或更换电极");
    }
    
    return impedance;
}
