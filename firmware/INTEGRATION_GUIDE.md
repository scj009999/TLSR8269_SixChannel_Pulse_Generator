# TLSR8269 六通道脉冲治疗仪 - 整合开发指南

> **版本**：v3.0  
> **日期**：2026-05-24  
> **状态**：主程序整合完成，待硬件验证

---

## 一、项目结构

```
TLSR8269_SixChannel_Pulse_Generator/
├── firmware/                          # 固件代码
│   ├── main_master_v3.c              # ✅ 整合主程序（21,402 bytes）
│   ├── tlsr8269_reg.h                # 寄存器定义
│   ├──
│   ├── i2c_driver.h/c                # ✅ I2C驱动（17,921 bytes）
│   ├── pwm_driver.h/c                # ✅ PWM驱动（24,393 bytes）
│   ├── adc_driver.h/c                # ✅ ADC驱动（24,451 bytes）
│   ├── lcd_driver.h/c                # ✅ LCD驱动（OLED 128x64）
│   ├── button_driver.h/c             # ✅ 按键驱动（8按键）
│   ├── ui_manager.h/c                # ✅ UI管理器
│   ├── self_check.c/h                # ✅ 自检模块
│   ├── INTEGRATION_GUIDE.md          # 本文件
│   └── DRIVER_GUIDE.md               # 驱动开发指南
├── hardware/                          # 硬件设计
│   ├── schematic.md                  # 原理图文档
│   ├── TLSR8269_PulseGenerator_Full_v3.kicad_sch  # KiCad原理图
│   ├── gerber/                        # Gerber文件
│   ├── BOM.csv                        # 物料清单
│   └── ...
└── docs/                              # 文档
    ├── MEDICAL_SAFETY.md
    ├── README.md
    └── ...
```

---

## 二、整合状态

### 2.1 已完成模块 ✅

| 模块 | 文件 | 大小 | 功能 |
|------|------|------|------|
| 主程序 | `main_master_v3.c` | 21,402 bytes | 系统初始化、治疗流程、安全监控 |
| 定时器 | `timer_driver.h/c` | 14,218 bytes | 系统Tick、延时、看门狗 |
| UART | `uart_driver.h/c` | 26,047 bytes | 串口通信、调试日志 |
| Flash | `flash_driver.h/c` | 16,434 bytes | 参数存储、日志记录 |
| I2C驱动 | `i2c_driver.h/c` | 17,921 bytes | I2C通信，设备扫描 |
| PWM驱动 | `pwm_driver.h/c` | 24,393 bytes | 6通道PWM输出，急停<10ms |
| ADC驱动 | `adc_driver.h/c` | 24,451 bytes | 阻抗/电流/电压检测 |
| LCD驱动 | `lcd_driver.h/c` | ~15,000 bytes | OLED显示 |
| 按键驱动 | `button_driver.h/c` | ~12,000 bytes | 8按键输入 |
| UI管理 | `ui_manager.h/c` | ~14,000 bytes | 界面管理 |
| 自检模块 | `self_check.c/h` | ~16,000 bytes | 系统自检 |
| **合计** | | **~201,866 bytes** | |

### 2.2 待实现模块 ⏳

| 模块 | 说明 | 优先级 |
|------|------|--------|
| `ble_driver` | 蓝牙通信 | 🟢 可选 |
| `mobile_app` | 手机APP | 🟢 可选 |

---

## 三、主程序架构

### 3.1 系统状态机

```
[INIT] → [SELF_TEST] → [STANDBY] → [CONFIG] → [READY] → [THERAPY]
                                          ↓         ↓         ↓
                                    [EMERGENCY] ← [FAULT] ← [PAUSED]
```

### 3.2 主循环时序

```
每1ms (系统Tick):
├── 按键扫描 (每10ms)
├── ADC扫描 (每100ms)
│   └── 更新治疗状态
├── UI刷新 (每100ms)
└── 安全检查 (每50ms)
    ├── 电流检查
    ├── 阻抗检查
    └── 时间检查
```

### 3.3 治疗流程

```
1. 待机 [STANDBY]
   ├── OK键 → 配置菜单
   └── START键 → 快速开始（默认参数）

2. 配置 [CONFIG]
   ├── 设置频率、占空比、时长
   ├── 选择活动通道
   └── 确认 → 就绪

3. 就绪 [READY]
   ├── START键 → 开始治疗
   └── 检查电极接触

4. 治疗中 [THERAPY]
   ├── 实时显示进度
   ├── 每50ms安全检查
   ├── CANCEL键 → 暂停
   └── STOP键/长按START → 急停

5. 暂停 [PAUSED]
   ├── START键 → 恢复
   └── STOP键 → 停止

6. 急停 [EMERGENCY]
   ├── 立即停止所有输出
   ├── 长按OK → 解除急停
   └── 返回待机
```

---

## 四、引脚分配汇总

### 4.1 完整引脚表

| 功能 | 引脚 | Arduino | 说明 |
|------|------|---------|------|
| **PWM输出** |
| CH0 | PA0 | D0 | PWM0 |
| CH1 | PA1 | D1 | PWM1 |
| CH2 | PA2 | D2 | PWM2 |
| CH3 | PA3 | D3 | PWM3 |
| CH4 | PB0 | D8 | PWM4 |
| CH5 | PB1 | D9 | PWM5 |
| **ADC输入** |
| ADC0 | PB0 | A0/D8 | 与PWM4复用 |
| ADC1 | PB1 | A1/D9 | 与PWM5复用 |
| ADC2 | PB2 | A2/D10 | **电位器推荐** |
| ADC3 | PB3 | A3/D11 | 通用ADC |
| ADC4 | PB4 | A4/D12 | 通用ADC |
| ADC5 | PB5 | A5/D13 | 通用ADC |
| **I2C通信** |
| SDA | PB6 | D14 | OLED, INA219 |
| SCL | PB7 | D15 | OLED, INA219 |
| **按键输入** |
| UP | PC0 | D16 | 上 |
| DOWN | PC1 | D17 | 下 |
| LEFT | PC2 | D18 | 左 |
| RIGHT | PC3 | D19 | 右 |
| OK | PC4 | D20 | 确认 |
| CANCEL | PC5 | D21 | 取消 |
| START | PD0 | D24 | 启动 |
| STOP | PD1 | D25 | 停止/急停 |

### 4.2 引脚冲突说明

| 引脚 | 功能A | 功能B | 解决方案 |
|------|-------|-------|----------|
| PB0 | PWM4 | ADC0 | 时分复用，治疗时PWM，检测时ADC |
| PB1 | PWM5 | ADC1 | 时分复用 |
| PB2 | ADC2 | - | 专用于电位器 |

---

## 五、医疗级安全机制

### 5.1 多层保护架构

```
Layer 1: 硬件保护
├── 电流限制电阻（每通道）
├── 光耦隔离（PC817×6）
├── 保险丝（总电源）
└── 急停按钮（硬件中断）

Layer 2: PWM驱动保护
├── 急停响应 <10ms
├── 硬件电流限制 20mA
├── 阻抗检测 500Ω-10kΩ
└── 故障分类和记录

Layer 3: ADC监控
├── 16次过采样
├── 过流检测（>20mA）
├── 开路检测（>10kΩ）
├── 短路检测（<500Ω）
└── 连续扫描模式

Layer 4: 主程序保护
├── 50ms周期安全检查
├── 电极接触验证
├── 治疗时间限制
├── 状态机保护
└── 看门狗定时器
```

### 5.2 急停流程

```
触发条件:
├── 用户按下STOP键
├── 长按START键（>1秒）
├── 检测到过流（>20mA）
├── 检测到短路
└── 硬件急停按钮

响应流程:
1. 立即执行 pwm_emergency_stop() (<10ms)
2. 禁用所有PWM输出
3. GPIO设为低电平
4. 设置 emergency_triggered 标志
5. 显示急停界面
6. 记录急停事件
7. 等待用户长按OK解除
```

---

## 六、编译和烧录

### 6.1 编译环境

```bash
# 使用Telink SDK
export TELINK_SDK=/path/to/telink_sdk

# 编译
make clean
make all

# 生成烧录文件
# output/TLSR8269_PulseTherapy.bin
```

### 6.2 烧录方法

```bash
# 使用Telink烧录器
# 1. 连接SWire接口（PA7）
# 2. 按住复位键
# 3. 执行烧录

make flash

# 或使用Python烧录工具
python3 telink_flash.py -p COM3 -b 921600 -i output/TLSR8269_PulseTherapy.bin
```

---

## 七、测试验证清单

### 7.1 上电测试

- [ ] 电源指示灯亮（3.3V）
- [ ] LCD显示启动画面
- [ ] 版本信息正确

### 7.2 自检测试

- [ ] I2C扫描检测到设备
- [ ] PWM测试通过（示波器检查）
- [ ] ADC测试通过（电位器变化）
- [ ] 按键测试通过

### 7.3 功能测试

- [ ] 配置界面正常
- [ ] 频率设置有效（1Hz-8MHz）
- [ ] 占空比设置有效（0-100%）
- [ ] 通道选择正常

### 7.4 治疗测试

- [ ] 启动治疗正常
- [ ] PWM输出波形正确
- [ ] 进度显示准确
- [ ] 暂停/恢复正常
- [ ] 停止正常

### 7.5 安全测试

- [ ] 急停响应<10ms
- [ ] 过流保护触发
- [ ] 开路检测有效
- [ ] 阻抗检测准确
- [ ] 时间到自动停止

---

## 八、已知问题

### 8.1 待修复

| 问题 | 影响 | 解决方案 |
|------|------|----------|
| `timer_driver` | 系统时钟 | ✅ 已完成（timer_driver.h/c） |
| `uart_driver` | 调试输出 | ✅ 已完成（uart_driver.h/c） |
| 看门狗 | 系统保护 | ✅ 已完成（timer_driver内） |
| Flash存储 | 参数保存 | ✅ 已完成（flash_driver.h/c） |
| BLE未实现 | 无无线通信 | 可选实现 |

### 8.2 硬件依赖

| 依赖 | 状态 | 说明 |
|------|------|------|
| SSD1306 OLED | 需采购 | I2C接口 |
| INA219 | 需采购 | I2C电流检测 |
| 电位器 | 需采购 | 10kΩ，5档 |
| 电极 | 需采购 | 氯化银或碳纤维 |

---

## 九、版本历史

| 版本 | 日期 | 说明 |
|------|------|------|
| v1.0 | 2026-05-07 | 初始版本，基础PWM |
| v2.0 | 2026-05-14 | 添加硬件设计 |
| v3.0 | 2026-05-24 | 整合所有驱动，完整治疗流程 |

---

## 十、下一步计划

### 立即行动
1. [ ] 实现 `timer_driver` — 系统时钟
2. [ ] 实现 `uart_driver` — 调试输出
3. [ ] 硬件验证 — 焊接测试板

### 短期目标
4. [ ] 看门狗实现
5. [ ] Flash参数存储
6. [ ] 完整安全测试

### 长期目标
7. [ ] BLE通信
8. [ ] 手机APP
9. [ ] 临床测试

---

*本指南随固件版本更新*
*医疗级设备，请严格遵循安全规范*
