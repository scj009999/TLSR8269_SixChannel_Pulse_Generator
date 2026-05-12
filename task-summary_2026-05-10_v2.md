# 任务总结：TLSR8269 开源经颅脉冲治疗仪 - KiCad导入准备

**日期**：2026-05-10  
**任务**：准备KiCad原理图文件和导入指南

---

## 一、用户确认信息

### 芯片型号确认
- **芯片**：TLSR8269F512ET32（QFN32，5×5mm）
- **内存**：32KB SRAM + 512KB Flash（与48脚相同）
- **决定**：使用32引脚版本（够用、更稳定、焊接更容易）

### 设计工具
- **原理图**：KiCad
- **PCB**：KiCad PCB Editor
- **下单**：嘉立创

---

## 二、新建文件

### 1. KiCad原理图文件
- **文件**：`hardware/TLSR8269_PulseGenerator_v2.kicad_sch`
- **内容**：基础框架（需要手动完善）
- **说明**：KiCad格式复杂，提供了基础框架，建议手动创建符号

### 2. KiCad导入指南
- **文件**：`hardware/KiCad_Import_Guide.md`
- **内容**：
  - 手动创建步骤（推荐）
  - 使用现有库方法
  - 替代芯片库方法
  - PCB设计建议
  - 导出Gerber步骤
  - 嘉立创下单流程

### 3. QFN32引脚定义
- **文件**：`hardware/TLSR8269_Pinout_QFN32.md`
- **内容**：
  - 32引脚完整分配表
  - 功能分组（PWM/ADC/I2C/UART）
  - 本项目用途映射
  - PCB布局建议

---

## 三、关键引脚分配（QFN32）

### PWM输出（6路）
```
PA0 (Pin 1)  → PWM0 → CH0
PA1 (Pin 2)  → PWM1 → CH1
PA2 (Pin 3)  → PWM2 → CH2
PA3 (Pin 4)  → PWM3 → CH3
PA4 (Pin 5)  → PWM4 → CH4
PA5 (Pin 6)  → PWM5 → CH5
```

### ADC输入（6路）
```
PC0 (Pin 11) → ADC0 → 电池电压监测
PC1 (Pin 12) → ADC1 → 电流检测 CH0
PC2 (Pin 13) → ADC2 → 电流检测 CH1
PC3 (Pin 14) → ADC3 → 电流检测 CH2
PC4 (Pin 15) → ADC4 → 电流检测 CH3
PC5 (Pin 16) → ADC5 → 电流检测 CH4
```

**注意**：QFN32只有6路ADC，如果需要6路电流检测+电池监测，建议使用INA219的I2C接口。

### I2C接口
```
PB0 (Pin 7)  → I2C_SCL → INA219
PB3 (Pin 10) → I2C_SDA → INA219
```

### UART调试
```
PB1 (Pin 8)  → UART_TX
PB2 (Pin 9)  → UART_RX
```

### 控制信号
```
PD0 (Pin 17) → 急停按钮（中断）
PD1 (Pin 18) → 状态LED
```

### 时钟和烧录
```
Pin 19-20 → 24MHz晶振
Pin 21    → SWS（烧录接口）
Pin 22    → RESET（复位）
```

---

## 四、KiCad导入步骤

### 步骤1：创建工程
1. 打开KiCad
2. File → New Project
3. 工程名：`TLSR8269_PulseGenerator`

### 步骤2：创建符号库
1. 打开Symbol Editor
2. 创建新库：`TLSR8269.lib`
3. 创建符号：`TLSR8269F512ET32`
4. 按引脚分配表添加32个引脚

### 步骤3：放置元件
按照 `schematic_v2.md` 中的电路图放置：
- 电源部分（LDO、保险丝、TVS）
- 隔离电源（B0505S-1W）
- 主控芯片（TLSR8269）
- 光耦阵列（PC817 × 6）
- 电流检测（INA219）
- 其他（按钮、LED、电容、电阻）

### 步骤4：连接线路
- 按 `schematic_v2.md` 连接
- 注意隔离区域划分
- 添加电源符号（+3.3V, +3.3V_ISO, GND, GND_ISO）

### 步骤5：ERC检查
- Tools → Electrical Rules Checker
- 修复所有错误

### 步骤6：生成网表
- Tools → Generate Netlist

---

## 五、PCB设计要点

### 层叠结构（4层板）
```
Top Layer:     信号层（TLSR8269、光耦输入侧）
Inner Layer 1: 地平面（GND）
Inner Layer 2: 电源平面（+3.3V, +3.3V_ISO）
Bottom Layer:  信号层（LDO、DC-DC、光耦输出侧）
```

### 关键布局
1. **隔离区域**：控制区 | 隔离带(≥3mm) | 患者区
2. **去耦电容**：尽量靠近VCC引脚
3. **光耦布局**：横跨隔离带
4. **晶振**：靠近XTAL引脚，走线短

---

## 六、嘉立创下单流程

### 准备文件
1. 导出Gerber（8个文件）
2. 导出钻孔文件（.drl）
3. 压缩为zip

### 上传下单
1. 登录嘉立创（jlc.com）
2. 上传Gerber zip文件
3. 确认参数：
   - 板厚：1.6mm
   - 层数：4层
   - 铜厚：1oz
   - 阻焊：绿色
   - 数量：5片

---

## 七、注意事项

### 设计注意
1. **引脚确认**：务必对照官方Datasheet
2. **散热焊盘**：QFN32底部散热焊盘需接地
3. **去耦电容**：每个VCC引脚旁至少100nF
4. **隔离距离**：光耦下方≥3mm

### 安全注意
1. **隔离耐压**：≥3000VDC
2. **限流保护**：每通道100Ω
3. **急停按钮**：常闭型，硬件中断
4. **保险丝**：500mA自恢复

---

## 八、文件清单（更新）

```
hardware/
├── schematic_v2.md                    # 完整原理图说明
├── PCB_CHECKLIST.md                   # 打板前检查清单
├── TLSR8269_Pinout_QFN32.md           # ⭐ QFN32引脚定义（新建）
├── KiCad_Import_Guide.md              # ⭐ KiCad导入指南（新建）
├── TLSR8269_PulseGenerator_v2.kicad_sch  # ⭐ KiCad原理图框架（新建）
└── (其他文件)
```

---

## 九、下一步行动

### 今天
- [ ] 在KiCad中创建工程
- [ ] 创建TLSR8269符号库
- [ ] 按引脚分配表添加引脚

### 明天
- [ ] 放置所有元件
- [ ] 连接线路
- [ ] 运行ERC检查

### 本周
- [ ] 完成PCB布局
- [ ] 导出Gerber
- [ ] 嘉立创下单

---

## 十、参考资料

1. `hardware/schematic_v2.md` - 完整原理图
2. `hardware/TLSR8269_Pinout_QFN32.md` - 引脚定义
3. `hardware/KiCad_Import_Guide.md` - 导入指南
4. `hardware/PCB_CHECKLIST.md` - 检查清单
5. Telink官方Wiki: https://wiki.telink-semi.cn/

---

*KiCad导入指南已准备好，可以开始创建原理图了！*
*32引脚版本完全够用，而且更稳定、更容易焊接。*
