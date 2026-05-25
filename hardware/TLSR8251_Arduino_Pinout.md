# TLSR8251F512ET32 Arduino Pinout 参考

> **来源**：DroneProject Arduino Core for Telink TLSR825x  
> **芯片型号**：TLSR8251F512ET32 (TLSR512 F3ET)  
> **封装**：QFN32  
> **适用性**：与 CR8269F512E132TKM1815 (TLSR8269) 高度兼容

---

## 一、GPIO 映射总览

### 1.1 可用 GPIO (24个)

| 端口 | GPIO编号 | Arduino引脚 | 功能 | 备注 |
|------|----------|-------------|------|------|
| **PA0** | GPIO 0 | D0 | PWM0 / INT0 | ✅ 硬件PWM |
| **PA1** | GPIO 1 | D1 | PWM1 / INT1 | ✅ 硬件PWM |
| **PA2** | GPIO 2 | D2 | PWM2 | ✅ 硬件PWM |
| **PA3** | GPIO 3 | D3 | PWM3 | ✅ 硬件PWM |
| **PA4** | GPIO 4 | D4 | SCK | SPI时钟 |
| **PA5** | GPIO 5 | D5 | MISO | SPI数据输入 |
| **PA6** | GPIO 6 | D6 | MOSI | SPI数据输出 |
| **PA7** | GPIO 7 | D7 | SS | SPI片选 |
| **PB0** | GPIO 8 | D8 / A0 | PWM4 / ADC0 | ✅ 硬件PWM + ADC |
| **PB1** | GPIO 9 | D9 / A1 | PWM5 / ADC1 | ✅ 硬件PWM + ADC |
| **PB2** | GPIO 10 | D10 / A2 | ADC2 | ADC输入 |
| **PB3** | GPIO 11 | D11 / A3 | ADC3 | ADC输入 |
| **PB4** | GPIO 12 | D12 / A4 | ADC4 | ADC输入 |
| **PB5** | GPIO 13 | D13 / A5 | ADC5 | ADC输入 |
| **PB6** | GPIO 14 | D14 | SDA / INT2 | I2C数据 |
| **PB7** | GPIO 15 | D15 | SCL / INT3 | I2C时钟 |
| **PC0** | GPIO 16 | D16 | RX | UART接收 |
| **PC1** | GPIO 17 | D17 | TX | UART发送 |
| **PC2** | GPIO 18 | D18 | - | 通用GPIO |
| **PC3** | GPIO 19 | D19 | - | 通用GPIO |
| **PC4** | GPIO 20 | D20 | - | 通用GPIO |
| **PC5** | GPIO 21 | D21 | - | 通用GPIO |
| **PC6** | GPIO 22 | D22 | - | 通用GPIO |
| **PC7** | GPIO 23 | D23 | LED_BUILTIN | 板载LED |

### 1.2 不可用端口

| 端口 | 状态 | 说明 |
|------|------|------|
| PD | ❌ 不可用 | QFN32未引出 |
| PE | ❌ 不可用 | QFN32未引出 |
| PF | ❌ 不可用 | QFN32未引出 |

---

## 二、硬件 PWM 引脚 (6路)

```cpp
// 只有这6个引脚支持硬件PWM
#define digitalPinHasPWM(p) ( \
    (p) == 0 ||  // PA0 - PWM0 \
    (p) == 1 ||  // PA1 - PWM1 \
    (p) == 2 ||  // PA2 - PWM2 \
    (p) == 3 ||  // PA3 - PWM3 \
    (p) == 8 ||  // PB0 - PWM4 \
    (p) == 9     // PB1 - PWM5 \
)
```

| PWM通道 | Arduino引脚 | 芯片引脚 | 功能 |
|---------|-------------|----------|------|
| PWM0 | D0 | PA0 | 脉冲输出通道0 |
| PWM1 | D1 | PA1 | 脉冲输出通道1 |
| PWM2 | D2 | PA2 | 脉冲输出通道2 |
| PWM3 | D3 | PA3 | 脉冲输出通道3 |
| PWM4 | D8 | PB0 | 脉冲输出通道4 |
| PWM5 | D9 | PB1 | 脉冲输出通道5 |

> **重要**：这与之前假设的 PA0-PA5 不同！实际硬件PWM分布在 PA0-PA3 和 PB0-PB1。

---

## 三、ADC 引脚 (6路)

```cpp
// ADC固定映射：A0-A5 → PB0~PB5 → GPIO 8~13
#define analogInputToDigitalPin(p) ((p < 6) ? (p + 8) : -1)
```

| ADC通道 | Arduino引脚 | 芯片引脚 | 功能 |
|---------|-------------|----------|------|
| ADC0 | A0 (D8) | PB0 | 模拟输入0 |
| ADC1 | A1 (D9) | PB1 | 模拟输入1 |
| ADC2 | A2 (D10) | PB2 | 模拟输入2 |
| ADC3 | A3 (D11) | PB3 | 模拟输入3 |
| ADC4 | A4 (D12) | PB4 | 模拟输入4 |
| ADC5 | A5 (D13) | PB5 | 模拟输入5 |

> **注意**：ADC和PWM在 PB0/PB1 上复用！

---

## 四、通信接口

### 4.1 SPI (固定引脚)

| 信号 | Arduino引脚 | 芯片引脚 |
|------|-------------|----------|
| SS | D7 | PA7 |
| MOSI | D6 | PA6 |
| MISO | D5 | PA5 |
| SCK | D4 | PA4 |

```cpp
static const uint8_t SS = 7;    // PA7
static const uint8_t MOSI = 6;  // PA6
static const uint8_t MISO = 5;  // PA5
static const uint8_t SCK = 4;   // PA4
```

### 4.2 I2C (固定引脚)

| 信号 | Arduino引脚 | 芯片引脚 | 中断 |
|------|-------------|----------|------|
| SDA | D14 | PB6 | INT2 |
| SCL | D15 | PB7 | INT3 |

```cpp
static const uint8_t SDA = 14;  // PB6
static const uint8_t SCL = 15;  // PB7
```

### 4.3 UART (固定引脚)

| 信号 | Arduino引脚 | 芯片引脚 |
|------|-------------|----------|
| RX | D16 | PC0 |
| TX | D17 | PC1 |

```cpp
static const uint8_t RX = 16;  // PC0
static const uint8_t TX = 17;  // PC1
```

---

## 五、中断引脚

| 中断 | Arduino引脚 | 芯片引脚 | 备注 |
|------|-------------|----------|------|
| INT0 | D0 | PA0 | 外部中断0 |
| INT1 | D1 | PA1 | 外部中断1 |
| INT2 | D14 | PB6 | I2C SDA |
| INT3 | D15 | PB7 | I2C SCL |

---

## 六、LED 引脚

```cpp
static const uint8_t LED_BUILTIN = 23;  // PC7
```

---

## 七、关键发现与影响

### 7.1 PWM引脚分布变化

**之前假设**：PA0-PA5 (6路PWM连续)
**实际分布**：PA0-PA3 + PB0-PB1 (不连续)

**影响**：
- 原理图需要调整：PWM4/PWM5 连接到 PB0/PB1 而非 PA4/PA5
- PCB走线需要重新规划
- 代码中的引脚定义需要更新

### 7.2 ADC/PWM复用冲突

**冲突引脚**：
- PB0 (D8): PWM4 + ADC0
- PB1 (D9): PWM5 + ADC1

**解决方案**：
- 使用ADC时，禁用对应PWM
- 或使用其他ADC通道 (PB2-PB5)
- 电位器连接到 PB2-PB5 而非 PB0-PB1

### 7.3 电位器连接建议

**原方案**：电位器 → PA4 (ADC)
**新方案**：电位器 → PB2 (ADC2) 或 PB3 (ADC3)

原因：
- PA4 用于 SPI SCK
- PB0/PB1 与PWM复用
- PB2-PB5 专用ADC，无冲突

---

## 八、更新后的引脚分配建议

### 8.1 六路脉冲输出

| 通道 | Arduino引脚 | 芯片引脚 | 功能 |
|------|-------------|----------|------|
| CH0 | D0 | PA0 | PWM0 |
| CH1 | D1 | PA1 | PWM1 |
| CH2 | D2 | PA2 | PWM2 |
| CH3 | D3 | PA3 | PWM3 |
| CH4 | D8 | PB0 | PWM4 |
| CH5 | D9 | PB1 | PWM5 |

### 8.2 其他功能

| 功能 | Arduino引脚 | 芯片引脚 | 备注 |
|------|-------------|----------|------|
| 电位器 | A2 (D10) | PB2 | ADC2，无冲突 |
| 急停按钮 | D18 | PC2 | 通用GPIO |
| 状态LED | D23 | PC7 | 板载LED |
| I2C SDA | D14 | PB6 | INA219 |
| I2C SCL | D15 | PB7 | INA219 |
| UART RX | D16 | PC0 | 调试 |
| UART TX | D17 | PC1 | 调试 |
| SPI SS | D7 | PA7 | 保留 |
| SPI MOSI | D6 | PA6 | 保留 |
| SPI MISO | D5 | PA5 | 保留 |
| SPI SCK | D4 | PA4 | 保留 |

---

## 九、代码更新建议

### 9.1 PulseTherapy.h 更新

```cpp
// 旧定义 (错误)
// #define CH0_PIN 0   // PA0
// #define CH1_PIN 1   // PA1
// #define CH2_PIN 2   // PA2
// #define CH3_PIN 3   // PA3
// #define CH4_PIN 4   // PA4 - 错误！PA4无PWM
// #define CH5_PIN 5   // PA5 - 错误！PA5无PWM

// 新定义 (正确)
#define CH0_PIN 0   // PA0 - PWM0
#define CH1_PIN 1   // PA1 - PWM1
#define CH2_PIN 2   // PA2 - PWM2
#define CH3_PIN 3   // PA3 - PWM3
#define CH4_PIN 8   // PB0 - PWM4
#define CH5_PIN 9   // PB1 - PWM5

// 电位器引脚更新
#define POT_PIN A2  // PB2 - ADC2 (原A0/PB0有冲突)
```

### 9.2 原理图更新

需要修改：
1. PWM4/PWM5 从 PA4/PA5 改为 PB0/PB1
2. 电位器从 PA4 改为 PB2
3. 确保 PB0/PB1 不连接需要ADC的电路

---

## 十、兼容性说明

### 10.1 TLSR8251 vs TLSR8269

| 特性 | TLSR8251 | TLSR8269 | 兼容性 |
|------|----------|----------|--------|
| 封装 | QFN32 | QFN32 | ✅ 相同 |
| Flash | 512KB | 512KB | ✅ 相同 |
| SRAM | 32KB | 32KB | ✅ 相同 |
| GPIO | 24 (PA/PB/PC) | 24 (PA/PB/PC) | ✅ 相同 |
| PWM | 6路 | 6路 | ✅ 相同 |
| ADC | 6路 | 6路 | ✅ 相同 |
| 蓝牙 | BLE 5.0 | BLE 5.0 | ✅ 相同 |

**结论**：引脚定义完全兼容，可直接使用。

---

## 十一、参考文件

- **来源**：Arduino-Telink 项目 (DroneProject)
- **GitHub**：搜索 "Arduino-Telink TLSR825x"
- **官方文档**：Telink Semiconductor TLSR8251 Datasheet

---

*此文档基于 Arduino Core 的 pins_arduino.h 文件整理*  
*实际使用时请以最新版Datasheet为准*
