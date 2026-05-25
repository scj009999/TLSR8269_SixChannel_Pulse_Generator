# ⚠️ 引脚分配修正通知

> **日期**：2026-05-23  
> **影响**：原理图、PCB、代码  
> **严重程度**：🔴 高 - 必须修正

---

## 一、问题发现

根据 Arduino-Telink Core (DroneProject) 的 `pins_arduino.h` 文件，**TLSR8251F512ET32 (QFN32)** 的硬件PWM引脚分布为：

```cpp
// ✅ 只有这6个引脚支持硬件PWM
#define digitalPinHasPWM(p) ( \
    (p) == 0 ||  // PA0 - PWM0 \
    (p) == 1 ||  // PA1 - PWM1 \
    (p) == 2 ||  // PA2 - PWM2 \
    (p) == 3 ||  // PA3 - PWM3 \
    (p) == 8 ||  // PB0 - PWM4 \
    (p) == 9     // PB1 - PWM5 \
)
```

### 原方案 (错误)

| 通道 | 引脚 | 问题 |
|------|------|------|
| CH0 | PA0 | ✅ 正确 |
| CH1 | PA1 | ✅ 正确 |
| CH2 | PA2 | ✅ 正确 |
| CH3 | PA3 | ✅ 正确 |
| CH4 | PA4 | ❌ **无PWM功能！** |
| CH5 | PA5 | ❌ **无PWM功能！** |

### 修正方案

| 通道 | 引脚 | Arduino | 状态 |
|------|------|---------|------|
| CH0 | PA0 | D0 | ✅ 硬件PWM0 |
| CH1 | PA1 | D1 | ✅ 硬件PWM1 |
| CH2 | PA2 | D2 | ✅ 硬件PWM2 |
| CH3 | PA3 | D3 | ✅ 硬件PWM3 |
| CH4 | PB0 | D8 | ✅ 硬件PWM4 |
| CH5 | PB1 | D9 | ✅ 硬件PWM5 |

---

## 二、影响范围

### 2.1 需要修改的文件

| 文件 | 修改内容 |
|------|----------|
| `hardware/TLSR8269_PulseGenerator_Full_v3.kicad_sch` | PWM4/5 从 PA4/PA5 改为 PB0/PB1 |
| `hardware/TLSR8269_PulseGenerator_Complete.kicad_sch` | 同上 |
| `PulseTherapy/PulseTherapy.h` | 引脚定义更新 |
| `PulseTherapy/PulseTherapy.cpp` | 引脚初始化更新 |
| `examples/ParkinsonTherapy.ino` | 引脚连接说明更新 |
| `hardware/BOM.csv` | 如有引脚变更影响元件 |
| `hardware/PickPlace.csv` | 坐标更新 |

### 2.2 电位器引脚变更

**原方案**：电位器 → PA4 (ADC)  
**问题**：PA4 用于 SPI SCK

**新方案**：电位器 → PB2 (ADC2)  
**原因**：
- PB0/PB1 与PWM复用
- PB2 专用ADC，无冲突
- Arduino引脚：A2 (D10)

---

## 三、修正后的完整引脚分配

### 3.1 六路脉冲输出

| 通道 | 芯片引脚 | Arduino | 功能 | 光耦 |
|------|----------|---------|------|------|
| CH0 | PA0 | D0 | PWM0 | U4 |
| CH1 | PA1 | D1 | PWM1 | U5 |
| CH2 | PA2 | D2 | PWM2 | U6 |
| CH3 | PA3 | D3 | PWM3 | U7 |
| CH4 | PB0 | D8 | PWM4 | U8 |
| CH5 | PB1 | D9 | PWM5 | U9 |

### 3.2 其他功能

| 功能 | 芯片引脚 | Arduino | 备注 |
|------|----------|---------|------|
| 电位器 | PB2 | A2 (D10) | ADC2 |
| 急停按钮 | PC2 | D18 | 通用GPIO |
| 状态LED | PC7 | D23 | LED_BUILTIN |
| I2C SDA | PB6 | D14 | INA219 |
| I2C SCL | PB7 | D15 | INA219 |
| UART RX | PC0 | D16 | 调试 |
| UART TX | PC1 | D17 | 调试 |
| SPI SS | PA7 | D7 | 保留 |
| SPI MOSI | PA6 | D6 | 保留 |
| SPI MISO | PA5 | D5 | 保留 |
| SPI SCK | PA4 | D4 | 保留 |

---

## 四、代码修改示例

### 4.1 PulseTherapy.h

```cpp
// 脉冲输出引脚 (修正后)
#define CH0_PIN 0   // PA0 - PWM0
#define CH1_PIN 1   // PA1 - PWM1
#define CH2_PIN 2   // PA2 - PWM2
#define CH3_PIN 3   // PA3 - PWM3
#define CH4_PIN 8   // PB0 - PWM4  ← 修正
#define CH5_PIN 9   // PB1 - PWM5  ← 修正

// 电位器引脚 (修正后)
#define POT_PIN A2  // PB2 - ADC2  ← 修正

// 其他引脚
#define ESTOP_PIN 18    // PC2 - 急停
#define LED_PIN 23      // PC7 - LED
```

### 4.2 引脚初始化

```cpp
void PulseTherapy::begin() {
    // 配置PWM引脚
    pinMode(CH0_PIN, OUTPUT);  // PA0
    pinMode(CH1_PIN, OUTPUT);  // PA1
    pinMode(CH2_PIN, OUTPUT);  // PA2
    pinMode(CH3_PIN, OUTPUT);  // PA3
    pinMode(CH4_PIN, OUTPUT);  // PB0 ← 修正
    pinMode(CH5_PIN, OUTPUT);  // PB1 ← 修正
    
    // 配置ADC引脚
    pinMode(POT_PIN, INPUT);   // PB2 ← 修正
    
    // ...其余初始化
}
```

---

## 五、PCB修改要点

### 5.1 顶层走线变更

1. **PWM4 (CH4)**
   - 从：PA4 → 光耦U8
   - 改为：PB0 → 光耦U8

2. **PWM5 (CH5)**
   - 从：PA5 → 光耦U9
   - 改为：PB1 → 光耦U9

3. **电位器**
   - 从：PA4
   - 改为：PB2

### 5.2 注意事项

- PB0/PB1 原用于ADC，现改为PWM输出
- 确保ADC功能不使用PB0/PB1
- 检查是否有其他电路占用PB0/PB1

---

## 六、验证清单

### 6.1 修改前检查

- [ ] 确认PA4/PA5无PWM功能
- [ ] 确认PB0/PB1有PWM功能
- [ ] 确认PB2可用作ADC

### 6.2 修改内容

- [ ] 更新原理图
- [ ] 更新PCB布局
- [ ] 更新代码引脚定义
- [ ] 更新文档
- [ ] 重新生成Gerber

### 6.3 修改后验证

- [ ] DRC检查通过
- [ ] 引脚无冲突
- [ ] 代码编译通过
- [ ] 功能测试正常

---

## 七、时间估算

| 任务 | 预计时间 |
|------|----------|
| 修改原理图 | 30分钟 |
| 调整PCB布局 | 1-2小时 |
| 更新代码 | 15分钟 |
| 重新生成Gerber | 15分钟 |
| 验证测试 | 30分钟 |
| **总计** | **2.5-3小时** |

---

## 八、参考文档

- `hardware/TLSR8251_Arduino_Pinout.md` - 完整引脚映射
- Arduino-Telink Core: `pins_arduino.h`
- Telink TLSR8251 Datasheet

---

> ⚠️ **重要**：此修正必须完成，否则CH4/CH5无PWM输出！

> 📅 **建议**：在提交PCB打样前完成此修正。
