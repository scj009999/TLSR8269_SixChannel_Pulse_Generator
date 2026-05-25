# 任务总结：TLSR8251 Arduino引脚映射分析与修正

**日期**：2026-05-23  
**任务**：分析Arduino-Telink Core引脚定义，发现PWM引脚分配错误并提供修正方案

---

## 一、关键发现

### 1.1 问题识别

根据Arduino-Telink Core (DroneProject) 的 `pins_arduino.h` 文件，发现**严重的引脚分配错误**：

**原方案 (错误)**：
- CH4 → PA4 (假设有PWM)
- CH5 → PA5 (假设有PWM)

**实际情况**：
- PA4/PA5 **无硬件PWM功能**
- 硬件PWM仅存在于：PA0-PA3, PB0-PB1

### 1.2 正确引脚分布

```cpp
#define digitalPinHasPWM(p) ( \
    (p) == 0 ||  // PA0 - PWM0 ✅ \
    (p) == 1 ||  // PA1 - PWM1 ✅ \
    (p) == 2 ||  // PA2 - PWM2 ✅ \
    (p) == 3 ||  // PA3 - PWM3 ✅ \
    (p) == 8 ||  // PB0 - PWM4 ✅ \
    (p) == 9     // PB1 - PWM5 ✅ \
)
```

---

## 二、影响分析

### 2.1 影响范围

| 方面 | 影响程度 | 说明 |
|------|----------|------|
| 原理图 | 🔴 高 | PWM4/5需从PA4/PA5改到PB0/PB1 |
| PCB布局 | 🔴 高 | 需要重新走线 |
| 代码 | 🟡 中 | 引脚定义需更新 |
| 文档 | 🟡 中 | 所有引脚相关文档需更新 |
| BOM | 🟢 低 | 元件不变，仅引脚变更 |

### 2.2 需要修改的文件

1. `hardware/TLSR8269_PulseGenerator_Full_v3.kicad_sch`
2. `hardware/TLSR8269_PulseGenerator_Complete.kicad_sch`
3. `PulseTherapy/PulseTherapy.h`
4. `PulseTherapy/PulseTherapy.cpp`
5. `examples/ParkinsonTherapy.ino`
6. `hardware/BOM.csv`
7. `hardware/PickPlace.csv`
8. `hardware/GERBER_GUIDE.md`

---

## 三、修正方案

### 3.1 六路PWM引脚 (修正后)

| 通道 | 芯片引脚 | Arduino | 功能 |
|------|----------|---------|------|
| CH0 | PA0 | D0 | PWM0 |
| CH1 | PA1 | D1 | PWM1 |
| CH2 | PA2 | D2 | PWM2 |
| CH3 | PA3 | D3 | PWM3 |
| CH4 | PB0 | D8 | PWM4 |
| CH5 | PB1 | D9 | PWM5 |

### 3.2 电位器引脚 (修正)

**原方案**：PA4 (与SPI SCK冲突)  
**新方案**：PB2 (ADC2，无冲突)

### 3.3 其他功能引脚

| 功能 | 芯片引脚 | Arduino |
|------|----------|---------|
| 急停按钮 | PC2 | D18 |
| 状态LED | PC7 | D23 |
| I2C SDA | PB6 | D14 |
| I2C SCL | PB7 | D15 |
| UART RX | PC0 | D16 |
| UART TX | PC1 | D17 |

---

## 四、生成的文档

### 4.1 新创建文件

| 文件 | 大小 | 说明 |
|------|------|------|
| `hardware/TLSR8251_Arduino_Pinout.md` | 6,116 bytes | 完整Arduino引脚映射参考 |
| `hardware/PINOUT_CORRECTION.md` | 3,918 bytes | 引脚修正通知与指南 |
| `task-summary_2026-05-23_v25.md` | 本文件 | 任务总结 |

### 4.2 文档内容

**TLSR8251_Arduino_Pinout.md** 包含：
- 24个GPIO完整映射表
- 6路硬件PWM引脚定义
- 6路ADC引脚定义
- SPI/I2C/UART固定引脚
- 中断引脚映射
- 关键发现与影响分析
- 更新后的引脚分配建议
- 代码更新示例
- 兼容性说明 (TLSR8251 vs TLSR8269)

**PINOUT_CORRECTION.md** 包含：
- 问题发现说明
- 影响范围分析
- 修正后的完整引脚分配
- 代码修改示例
- PCB修改要点
- 验证清单
- 时间估算

---

## 五、兼容性确认

### 5.1 TLSR8251 vs TLSR8269

| 特性 | TLSR8251 | TLSR8269 | 兼容性 |
|------|----------|----------|--------|
| 封装 | QFN32 | QFN32 | ✅ 相同 |
| Flash | 512KB | 512KB | ✅ 相同 |
| SRAM | 32KB | 32KB | ✅ 相同 |
| GPIO | 24 | 24 | ✅ 相同 |
| PWM | 6路 | 6路 | ✅ 相同 |
| ADC | 6路 | 6路 | ✅ 相同 |
| 蓝牙 | BLE 5.0 | BLE 5.0 | ✅ 相同 |

**结论**：引脚定义完全兼容，可直接使用TLSR8251的Arduino Core。

---

## 六、下一步行动

### 6.1 立即行动 (必须)

1. **修改原理图**
   - PWM4: PA4 → PB0
   - PWM5: PA5 → PB1
   - 电位器: PA4 → PB2

2. **更新PCB布局**
   - 重新走线PWM4/5
   - 调整电位器连接

3. **更新代码**
   - PulseTherapy.h 引脚定义
   - PulseTherapy.cpp 初始化

### 6.2 验证测试

1. DRC检查
2. 引脚冲突检查
3. 代码编译测试
4. 功能验证

### 6.3 重新生成

1. 重新生成Gerber文件
2. 更新BOM和PickPlace
3. 提交PCB打样

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

## 八、重要提醒

### 8.1 安全警告

⚠️ **此修正必须完成，否则CH4/CH5无PWM输出！**  
⚠️ **在提交PCB打样前务必完成修正！**

### 8.2 设计原则

1. 严格对照Arduino Core引脚定义
2. 确认每个引脚的功能
3. 避免功能复用冲突
4. 保留测试点便于调试

---

## 九、参考资源

- Arduino-Telink Core (DroneProject)
- `hardware/TLSR8251_Arduino_Pinout.md`
- `hardware/PINOUT_CORRECTION.md`
- Telink TLSR8251/8269 Datasheet

---

*发现及时，修正成本可控。建议在PCB打样前完成所有修改。*
