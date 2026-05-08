/**
 * PulseTherapy - 开源经颅脉冲治疗库
 * 
 * 为帕金森患者设计的 Arduino 库
 * 基于 Telink TLSR8269 六路PWM脉冲发生器
 * 
 * 项目主页: https://github.com/scj009999/TLSR-8269-8266-8232-ardunio-
 * 许可证: GPL v3
 */

#ifndef PULSE_THERAPY_H
#define PULSE_THERAPY_H

#include "Arduino.h"

// 版本信息
#define PULSE_THERAPY_VERSION_MAJOR 1
#define PULSE_THERAPY_VERSION_MINOR 0
#define PULSE_THERAPY_VERSION_PATCH 0

// 安全参数限制（基于临床研究）
#define SAFE_FREQ_MIN       1       // Hz
#define SAFE_FREQ_MAX       200     // Hz
#define SAFE_DUTY_MIN       0       // 0.01%
#define SAFE_DUTY_MAX       10000   // 100.00%
#define SAFE_CURRENT_MAX    5000    // uA (5mA)
#define SAFE_DURATION_MAX   30      // minutes

// 通道数量
#define PULSE_CHANNEL_NUM   6

// 引脚定义（TLSR8269）
#define CH0_PIN     0   // PA0 - PWM0
#define CH1_PIN     1   // PA1 - PWM1
#define CH2_PIN     2   // PA2 - PWM2
#define CH3_PIN     3   // PA3 - PWM3
#define CH4_PIN     4   // PA4 - PWM4
#define CH5_PIN     5   // PA5 - PWM5

// 治疗模式预设
enum TherapyMode {
    MODE_CUSTOM = 0,           // 自定义
    MODE_PARKINSON_STD,        // 帕金森标准（刘河生方案）
    MODE_PARKINSON_INTENSE,    // 帕金森强化
    MODE_PARKINSON_CONSERVATIVE, // 保守方案
    MODE_SLEEP,                // 助眠模式
    MODE_PAIN_RELIEF           // 疼痛缓解
};

// 通道状态
struct ChannelState {
    bool enabled;
    uint32_t frequency;
    uint16_t duty;          // 0-10000 (0.00% - 100.00%)
    uint16_t phase;         // 0-36000 (0.00° - 360.00°)
    uint16_t current_uA;    // 输出电流（微安）
};

// 回调函数类型
typedef void (*PulseCallback)(uint8_t channel, bool state);
typedef void (*SafetyCallback)(const char* message);

class PulseTherapy {
public:
    // 构造函数
    PulseTherapy();
    
    // 初始化
    bool begin();
    void end();
    
    // 通道控制
    bool configureChannel(uint8_t ch, uint32_t freq, uint16_t duty, uint16_t phase = 0);
    bool enableChannel(uint8_t ch);
    bool disableChannel(uint8_t ch);
    bool enableAll();
    bool disableAll();
    
    // 参数调整
    bool setFrequency(uint8_t ch, uint32_t freq);
    bool setDuty(uint8_t ch, uint16_t duty);
    bool setPhase(uint8_t ch, uint16_t phase);
    bool setCurrentLimit(uint16_t current_uA);
    
    // 治疗模式
    bool startTherapy(TherapyMode mode, uint8_t duration_minutes);
    bool stopTherapy();
    bool pauseTherapy();
    bool resumeTherapy();
    
    // 预设模式
    bool loadParkinsonStandard();      // 40Hz, 50%, 20min
    bool loadParkinsonIntense();       // 40Hz, 70%, 30min
    bool loadParkinsonConservative();  // 20Hz, 30%, 15min
    bool loadSleepMode();              // 10Hz, 30%, 60min
    bool loadPainRelief();             // 100Hz, 50%, 20min
    
    // 安全功能
    bool emergencyStop();
    bool safetyCheck();
    bool isSafe() { return _safetyPassed; }
    
    // 状态查询
    ChannelState getChannelState(uint8_t ch);
    uint8_t getActiveChannelCount();
    uint32_t getElapsedTime();          // 秒
    uint32_t getRemainingTime();        // 秒
    bool isRunning() { return _therapyRunning; }
    
    // 回调设置
    void onChannelChange(PulseCallback cb) { _channelCallback = cb; }
    void onSafetyAlert(SafetyCallback cb) { _safetyCallback = cb; }
    void onComplete(void (*cb)()) { _completeCallback = cb; }
    
    // 更新（需在loop中调用）
    void update();
    
    // 呼吸效果（指示用）
    void breathingEffect(uint8_t ch, float period_sec);
    
    // 版本信息
    static const char* getVersion();

private:
    ChannelState _channels[PULSE_CHANNEL_NUM];
    bool _initialized;
    bool _therapyRunning;
    bool _therapyPaused;
    bool _safetyPassed;
    uint16_t _currentLimit;
    uint32_t _therapyStartTime;
    uint32_t _therapyDuration;
    TherapyMode _currentMode;
    
    PulseCallback _channelCallback;
    SafetyCallback _safetyCallback;
    void (*_completeCallback)();
    
    bool validateParameters(uint8_t ch, uint32_t freq, uint16_t duty);
    bool checkHardware();
    void applySafetyLimits(uint8_t ch);
};

// 全局实例
extern PulseTherapy Pulse;

#endif // PULSE_THERAPY_H
