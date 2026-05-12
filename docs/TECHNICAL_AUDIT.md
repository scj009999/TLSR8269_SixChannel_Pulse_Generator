# 技术审计报告 - 开源经颅脉冲治疗仪

**审计时间**：2026-05-09  
**审计版本**：v1.0  
**审计人员**：技术审查  

---

## 一、审计摘要

### 1.1 总体评价

本项目技术方案**基本可行**，但存在若干需要修复的安全隐患和优化空间。核心代码结构清晰，安全机制设计合理，但在电流检测精度、阻抗测量、紧急停止等方面需要改进。

### 1.2 风险等级

| 风险项 | 等级 | 状态 |
|--------|------|------|
| 电流检测精度不足 | 🔴 高 | 已修复 |
| 紧急停止非原子操作 | 🔴 高 | 已修复 |
| TLSR8269寄存器兼容性 | 🟡 中 | 待验证 |
| 阻抗检测逻辑缺陷 | 🟡 中 | 已修复 |
| 缺少医疗级绝缘 | 🟡 中 | 待硬件改进 |
| 图形化编程安全提示 | 🟢 低 | 建议改进 |
| 看门狗喂狗机制 | 🟢 低 | 已优化 |

---

## 二、已修复问题

### 2.1 紧急停止优化 ✅

**问题**：紧急停止操作可能被中断打断，导致部分通道未关闭。

**修复**：使用`noInterrupts()`/`interrupts()`确保原子操作。

```cpp
bool PulseTherapy::emergencyStop() {
    noInterrupts();  // 关中断
    analogWrite(PWM_EN, 0);
    for (int i = 0; i < PULSE_CHANNEL_NUM; i++) {
        digitalWrite(CH0_PIN + i, LOW);
        _channels[i].enabled = false;
    }
    _therapyRunning = false;
    _therapyPaused = false;
    interrupts();  // 开中断
    // ...
}
```

### 2.2 电流检测精度优化 ✅

**问题**：单次采样精度不足，易受噪声干扰。

**修复**：8次采样取平均，缩短延时。

```cpp
uint16_t PulseTherapy::readChannelCurrent(uint8_t ch) {
    uint32_t sum = 0;
    const uint8_t samples = 8;
    for (uint8_t i = 0; i < samples; i++) {
        analogWrite(ADC_CTRL, 0x80 | adc_ch);
        delayMicroseconds(100);
        sum += analogRead(ADC_DATA);
    }
    uint16_t adc_avg = sum / samples;
    return ADC_TO_UA(adc_avg);
}
```

### 2.3 实时电流监测 ✅

**问题**：安全检查间隔1秒，电流超限响应慢。

**修复**：增加每50ms的实时电流监测，超限自动降低占空比。

```cpp
// 实时电流监测（每通道每50ms）
static uint8_t currentCh = 0;
static uint32_t lastCurrentCheck = 0;
if (now - lastCurrentCheck > 50) {
    lastCurrentCheck = now;
    if (_channels[currentCh].enabled) {
        uint16_t current = readChannelCurrent(currentCh);
        if (current > _currentLimit) {
            uint16_t newDuty = (_channels[currentCh].duty * _currentLimit) / current;
            setDuty(currentCh, newDuty);
        }
    }
    currentCh = (currentCh + 1) % PULSE_CHANNEL_NUM;
}
```

### 2.4 阻抗检测改进 ✅

**问题**：开路判断阈值不合理，未区分不同阻抗状态。

**修复**：优化阈值，增加状态提示。

```cpp
uint16_t PulseTherapy::readElectrodeImpedance(uint8_t ch) {
    // ...
    if (impedance > 50000) {
        // 高阻抗：可能电极脱落或干燥
        if (_safetyCallback) _safetyCallback("电极阻抗过高，请检查连接或湿润电极");
    } else if (impedance > 10000) {
        // 中等阻抗：可能皮肤干燥
        if (_safetyCallback) _safetyCallback("电极阻抗偏高，建议清洁皮肤或更换电极");
    }
    return impedance;
}
```

### 2.5 安全检查频率提升 ✅

**问题**：安全检查每秒一次，响应不够及时。

**修复**：改为每100ms检查一次。

```cpp
if (now - lastSafetyCheck > 100) {  // 每100ms检查一次
    lastSafetyCheck = now;
    if (!safetyCheck()) {
        emergencyStop();
        return;
    }
}
```

---

## 三、待修复问题

### 3.1 TLSR8269寄存器兼容性 🟡

**问题**：代码使用`analogWrite`/`analogRead`直接操作寄存器，可能与Telink Arduino Core不兼容。

**建议**：
1. 确认使用的是Telink Arduino Core（https://github.com/AaronLiang/telink_arduino）
2. 或改用标准Telink SDK API
3. 添加编译时检查：

```cpp
#ifndef TELINK_ARDUINO_CORE
  #warning "请确认使用Telink Arduino Core，否则寄存器操作可能不兼容"
#endif
```

### 3.2 缺少医疗级绝缘设计 🟡

**问题**：原理图中无隔离DC-DC，患者电路与地未隔离。

**建议**：
1. 增加光耦隔离 + 隔离DC-DC模块（如B0505S-1W）
2. 参考 `MEDICAL_SAFETY.md` 中的隔离架构
3. 进行漏电流测试（要求<10µA）

### 3.3 图形化编程安全提示 🟢

**问题**：Mixly积木无范围限制提示。

**建议**：
```javascript
// 在blocks.js中添加范围验证
Blockly.Blocks['pulse_set_frequency'] = {
  init: function() {
    this.appendValueInput("FREQ")
        .setCheck("Number")
        .appendField("设置频率(Hz)");
    // 添加范围限制
    this.setTooltip("频率范围: 1-200Hz");
  }
};
```

---

## 四、硬件改进建议

### 4.1 电流检测芯片

**建议**：使用INA219 I2C电流检测芯片替代采样电阻方案。

**优势**：
- 精度更高（±0.5%）
- 内置ADC，不占用主控ADC通道
- I2C接口，易于集成

### 4.2 隔离设计

**建议**：增加以下隔离措施：

```
电池 → 隔离DC-DC → 患者电路
       ↓
    光耦隔离 → 控制电路
```

**器件推荐**：
- 隔离DC-DC：B0505S-1W（¥2-3）
- 光耦：PC817 × 6（¥0.3 × 6）

### 4.3 电极材料

**建议**：使用医用电极材料，通过生物相容性测试。

---

## 五、软件改进建议

### 5.1 增加CRC校验

对治疗参数增加校验，防止内存损坏：

```cpp
struct ChannelState {
    bool enabled;
    uint32_t frequency;
    uint16_t duty;
    uint16_t phase;
    uint16_t current_uA;
    uint16_t crc;  // 新增：参数CRC校验
};
```

### 5.2 增加日志记录

记录治疗过程，便于分析：

```cpp
struct TherapyLog {
    uint32_t timestamp;
    uint8_t channel;
    uint16_t current_uA;
    uint16_t impedance;
    uint16_t duty;
};
```

### 5.3 增加自检模式

开机时进行完整自检：

```cpp
bool selfTest() {
    // 1. 检查PWM输出
    // 2. 检查ADC精度
    // 3. 检查电流检测
    // 4. 检查急停按钮
    // 5. 检查电极连接
}
```

---

## 六、合规性检查

### 6.1 与前期文档一致性

| 前期声明 | 当前实现 | 状态 |
|---------|---------|------|
| 物料成本¥65 | BOM清单9类器件 | ✅ 基本符合 |
| 电流≤2mA | SAFE_CURRENT_MAX=5000uA | ⚠️ 库限制5mA，示例限制2mA |
| 光耦隔离5000V | 原理图无隔离设计 | ❌ **待改进** |
| 急停按钮硬件中断 | attachInterrupt | ✅ 已实现 |
| 阻抗检测<10kΩ | readElectrodeImpedance | ✅ 已实现 |
| 蓝牙BLE连接 | 无相关代码 | ❌ **待实现** |

### 6.2 医疗标准参考

- GB 9706.1-2020 医用电气设备安全通用要求
- YY 0505-2012 医用电气设备电磁兼容
- YY/T 0696-2008 神经和肌肉刺激器

---

## 七、下一步行动计划

### 7.1 立即行动（本周）
- [ ] 验证TLSR8269 Arduino Core兼容性
- [ ] 补充光耦隔离电路到原理图
- [ ] 编写单元测试（模拟安全场景）

### 7.2 短期行动（本月）
- [ ] 实现BLE通信模块
- [ ] 增加INA219电流检测芯片
- [ ] 制作PCB原型

### 7.3 中期行动（3个月内）
- [ ] 进行电气安全测试
- [ ] 进行生物相容性测试
- [ ] 申请医疗器械注册检验

---

## 八、审计结论

本项目技术方案**基本可行**，安全机制设计合理，但需要在以下方面改进：

1. **硬件**：补充隔离设计，使用专用电流检测芯片
2. **软件**：已修复紧急停止、电流检测等关键问题
3. **合规**：需进行医疗器械注册检验

**建议**：在修复上述问题后，可进行小规模临床验证。

---

*审计完成时间：2026-05-09*  
*审计版本：v1.0*  
*下次审计建议：硬件原型完成后*
