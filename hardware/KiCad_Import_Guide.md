# KiCad 导入指南

## 文件说明

由于KiCad格式复杂，我为您准备了以下文件：

1. **TLSR8269_PulseGenerator_v2.kicad_sch** - KiCad原理图文件（基础框架）
2. **schematic_v2.md** - 完整原理图说明（Markdown格式）

## 推荐导入方法

### 方法一：手动创建（推荐）

由于TLSR8269不是标准库元件，建议手动创建：

#### 步骤1：创建新工程
1. 打开KiCad
2. File → New Project → 选择文件夹
3. 工程名：`TLSR8269_PulseGenerator`

#### 步骤2：创建原理图符号库

**TLSR8269F512ET32 (QFN32)**

```
引脚分配（基于Telink官方资料）：

Pin 1:   PA0  - PWM0 / GPIO
Pin 2:   PA1  - PWM1 / GPIO
Pin 3:   PA2  - PWM2 / GPIO
Pin 4:   PA3  - PWM3 / GPIO
Pin 5:   PA4  - PWM4 / GPIO
Pin 6:   PA5  - PWM5 / GPIO
Pin 7:   PB0  - GPIO / I2C_SCL
Pin 8:   PB1  - UART_TX / GPIO
Pin 9:   PB2  - UART_RX / GPIO
Pin 10:  PB3  - GPIO / I2C_SDA
Pin 11:  PC0  - ADC0 / GPIO
Pin 12:  PC1  - ADC1 / GPIO
Pin 13:  PC2  - ADC2 / GPIO
Pin 14:  PC3  - ADC3 / GPIO
Pin 15:  PC4  - ADC4 / GPIO
Pin 16:  PC5  - ADC5 / GPIO
Pin 17:  PD0  - GPIO / 中断
Pin 18:  PD1  - GPIO / PWM
Pin 19:  XTAL - 24MHz晶振输入
Pin 20:  XTAL - 24MHz晶振输出
Pin 21:  SWS  - 单线调试接口
Pin 22:  RESET- 复位（低电平有效）
Pin 23:  VCC  - 3.3V电源
Pin 24:  VCC  - 3.3V电源
Pin 25:  GND  - 地
Pin 26:  GND  - 地
Pin 27:  GND  - 地
Pin 28:  GND  - 地
Pin 29:  NC   - 悬空
Pin 30:  NC   - 悬空
Pin 31:  NC   - 悬空
Pin 32:  NC   - 悬空
```

**注意**：以上引脚分配为参考，具体请以Telink官方Datasheet为准。

#### 步骤3：创建符号

1. 打开Symbol Editor
2. File → New Library → 创建 `TLSR8269.lib`
3. 创建新符号：`TLSR8269F512ET32`
4. 添加引脚（按上述分配）
5. 保存

#### 步骤4：放置元件

按照 `schematic_v2.md` 中的电路图，放置以下元件：

**电源部分：**
- J1: USB接口或电池接口
- F1: 保险丝 500mA
- D1: TVS管 5V
- U2: AMS1117-3.3 (LDO)
- C1-C4: 电容 (10µF, 100nF)

**隔离电源：**
- U3: B0505S-1W (隔离DC-DC)
- C5-C8: 电容

**主控：**
- U1: TLSR8269F512ET32
- Y1: 24MHz晶振
- C9-C10: 22pF负载电容
- C11-C16: 100nF去耦电容

**光耦隔离（×6）：**
- U4-U9: PC817
- R2-R7: 330Ω (光耦输入限流)
- R8-R13: 100Ω (患者侧限流)

**电流检测：**
- U10: INA219
- R14-R19: 1Ω采样电阻（可选，如果用INA219内置采样电阻则不需要）

**其他：**
- SW1: 复位按钮
- SW2: 急停按钮（常闭）
- D2-D4: LED (红/绿/黄)
- R20-R22: 1kΩ LED限流
- R23: 10kΩ 上拉电阻

#### 步骤5：连接线路

按照 `schematic_v2.md` 中的连接关系，完成布线。

#### 步骤6：添加电源符号

- +3.3V (控制电路)
- +3.3V_ISO (隔离电源)
- GND (系统地)
- GND_ISO (隔离地)

#### 步骤7：ERC检查

1. Tools → Electrical Rules Checker
2. 修复所有错误和警告

#### 步骤8：生成网表

1. Tools → Generate Netlist
2. 保存网表文件

---

### 方法二：使用现有库

#### 搜索现有库

1. 在KiCad中打开Symbol Editor
2. 搜索是否有Telink相关库
3. 如果没有，可以：
   - 在GitHub搜索 "KiCad TLSR8269"
   - 在SnapEDA搜索 "TLSR8269"

#### 导入外部库

1. 下载库文件 (.lib 或 .kicad_sym)
2. Preferences → Manage Symbol Libraries
3. 添加库文件路径

---

### 方法三：使用替代芯片库

如果找不到TLSR8269库，可以：

1. 找一个类似的QFN32芯片库
2. 修改引脚名称和编号
3. 保存为新符号

---

## PCB设计建议

### 层叠结构（4层板）

```
Top Layer:    信号层（TLSR8269、光耦输入侧）
Inner Layer 1: 地平面（GND）
Inner Layer 2: 电源平面（+3.3V, +3.3V_ISO）
Bottom Layer:  信号层（LDO、DC-DC、光耦输出侧）
```

### 关键布局要点

1. **隔离区域划分**
   - 控制电路区（系统地）
   - 隔离带（≥3mm宽度）
   - 患者电路区（隔离地）

2. **去耦电容**
   - 尽量靠近芯片VCC引脚
   - 接地回路短

3. **光耦布局**
   - 横跨隔离带
   - 输入侧靠近TLSR8269
   - 输出侧靠近输出端子

4. **晶振**
   - 靠近XTAL引脚
   - 走线短
   - 下方无其他信号线

---

## 导出Gerber

### 设置

1. File → Plot
2. 选择层：
   - F.Cu (顶层)
   - In1.Cu (内层1)
   - In2.Cu (内层2)
   - B.Cu (底层)
   - F.SilkS (顶层丝印)
   - B.SilkS (底层丝印)
   - F.Mask (顶层阻焊)
   - B.Mask (底层阻焊)
   - Edge.Cuts (板边)
3. 格式：Gerber RS-274X
4. 输出目录：gerber/

### 钻孔文件

1. File → Fabrication Outputs → Drill Files
2. 格式：Excellon
3. 输出到同一目录

---

## 嘉立创下单

### 准备文件

1. 压缩gerber目录为zip文件
2. 包含所有.gbr和.drl文件

### 上传

1. 登录嘉立创（https://www.jlc.com/）
2. 进入PCB下单页面
3. 上传Gerber文件
4. 确认参数：
   - 板厚：1.6mm
   - 层数：4层
   - 铜厚：1oz
   - 阻焊：绿色
   - 丝印：白色
   - 数量：5片

---

## 注意事项

1. **引脚确认**：请务必对照Telink官方Datasheet确认引脚定义
2. **封装选择**：QFN32 5×5mm，注意散热焊盘
3. **隔离距离**：光耦下方保持3mm间距
4. **测试点**：添加关键测试点，方便调试

---

## 参考文档

- `schematic_v2.md` - 完整原理图说明
- `PCB_CHECKLIST.md` - 打板前检查清单
- Telink官方Wiki: https://wiki.telink-semi.cn/

---

*如有问题，请参考KiCad官方文档或社区论坛。*
