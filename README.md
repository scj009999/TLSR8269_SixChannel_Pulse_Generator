# TLSR8269 六路脉冲发生器

## 项目概述

基于 **Telink TLSR8269F512** 芯片的六路独立脉冲发生器，使用6个模拟数字信号端口（PA0-PA5）作为脉冲输出。

## 硬件特性

- **主控芯片**: Telink TLSR8269F512
- **核心**: 32位MCU，最高48MHz
- **PWM通道**: 6路硬件PWM (PWM0-PWM5)
- **输出引脚**: PA0, PA1, PA2, PA3, PA4, PA5
- **工作电压**: 1.9V ~ 3.6V (推荐3.3V)

## 功能特性

| 参数 | 范围 | 说明 |
|------|------|------|
| 频率 | 1Hz ~ 8MHz | 受PWM时钟限制 |
| 占空比 | 0% ~ 100% | 精度0.01% |
| 相位 | 0° ~ 360° | 软件实现 |
| 输出电平 | 0V / VCC | CMOS输出 |

## 引脚定义

```
TLSR8269 (QFN48)
═══════════════════════════════════════

         ┌─────────────────┐
    PA0  │ 1            48 │ PA5      ← CH5
    PA1  │ 2            47 │ VCC
    PA2  │ 3            46 │ GND
    PA3  │ 4            45 │ PC0
    PA4  │ 5            44 │ PC1
    PA5  │ 6            43 │ PC2
    PB0  │ 7            42 │ PC3
    PB1  │ 8            41 │ PC4
    PB2  │ 9            40 │ PC5
    PB3  │ 10           39 │ PC6
    PB4  │ 11           38 │ PC7
    PB5  │ 12           37 │ PD0
    PB6  │ 13           36 │ PD1
    PB7  │ 14           35 │ PD2
    PC0  │ 15           34 │ PD3
    PC1  │ 16           33 │ PD4
    PC2  │ 17           32 │ PD5
    PC3  │ 18           31 │ PD6
    PC4  │ 19           30 │ PD7
    PC5  │ 20           29 │ PE0
    PC6  │ 21           28 │ PE1
    PC7  │ 22           27 │ PE2
    PD0  │ 23           26 │ PE3
    PD1  │ 24           25 │ PF0
         └─────────────────┘

脉冲输出引脚：
  CH0 → PA0 (Pin 1)  - PWM0
  CH1 → PA1 (Pin 2)  - PWM1
  CH2 → PA2 (Pin 3)  - PWM2
  CH3 → PA3 (Pin 4)  - PWM3
  CH4 → PA4 (Pin 5)  - PWM4
  CH5 → PA5 (Pin 6)  - PWM5

调试串口：
  TX  → PB1 (Pin 8)  - 115200bps
```

## 软件架构

```
main.c
├── pulse_generator_init()      // 初始化六路PWM
├── pulse_channel_config()      // 配置单通道参数
├── pulse_channel_enable()      // 使能通道输出
├── pulse_channel_disable()     // 禁止通道输出
├── pulse_set_frequency()       // 设置频率
├── pulse_set_duty()            // 设置占空比
├── pulse_set_phase()           // 设置相位
└── pulse_all_channels_update() // 批量更新
```

## 使用示例

### 基础配置

```c
// 初始化
pulse_generator_init();

// 配置通道0: 1kHz, 50%占空比
pulse_channel_config(0, 1000, 5000, 0);

// 配置通道1: 2kHz, 25%占空比
pulse_channel_config(1, 2000, 2500, 0);

// 使能输出
pulse_channel_enable(0);
pulse_channel_enable(1);
```

### 动态调整

```c
// 修改通道0频率为5kHz
pulse_set_frequency(0, 5000);

// 修改通道0占空比为75%
pulse_set_duty(0, 7500);

// 关闭通道0
pulse_channel_disable(0);
```

## 编译与烧录

### 环境要求

- Telink SDK (TLSR826x BLE SDK V3.3.1+)
- Eclipse IDE 或 Makefile
- Telink Burning and Debugging Tool (BDT)

### 编译步骤

```bash
# 1. 导入工程到Eclipse
# 2. 选择芯片型号: TLSR8269F512
# 3. 编译项目
make

# 4. 生成烧录文件
# 输出: firmware.bin
```

### 烧录步骤

```bash
# 使用Telink BDT工具
# 1. 连接EVK烧录器
# 2. 选择芯片型号 TLSR8269
# 3. 加载 firmware.bin
# 4. 点击 Download
```

## 注意事项

1. **低功耗冲突**: PWM与Power Management模式冲突，使用PWM时必须禁用PM
2. **频率限制**: 最高频率受PWM时钟限制，16MHz时钟下最高约8MHz
3. **占空比精度**: cycle值越大精度越高，低频时精度更好
4. **GPIO复用**: 确保PWM引脚未被其他功能占用
5. **电源去耦**: 建议在VCC引脚添加100nF去耦电容

## 频率与精度对照表

| 目标频率 | PWM时钟 | Cycle值 | 实际频率 | 误差 |
|----------|---------|---------|----------|------|
| 100Hz    | 16MHz   | 160000  | 100Hz    | 0%   |
| 1kHz     | 16MHz   | 16000   | 1kHz     | 0%   |
| 10kHz    | 16MHz   | 1600    | 10kHz    | 0%   |
| 100kHz   | 16MHz   | 160     | 100kHz   | 0%   |
| 1MHz     | 16MHz   | 16      | 1MHz     | 0%   |

## 扩展功能

- [ ] 串口命令控制
- [ ] 脉冲计数功能
- [ ] 外部触发同步
- [ ] 脉冲串模式 (Burst)
- [ ] 占空比扫描模式

## 参考资料

- [Telink TLSR8269 Datasheet](https://wiki.telink-semi.cn/doc/ds/DS_TLSR8269F512-E_Datasheet.pdf)
- [Telink 826x SDK Developer Handbook](https://wiki.telink-semi.cn/doc/an/AN_17092700-E_Telink%20826x%20BLE%20SDK%20Developer%20Handbook.pdf)
- [Telink Wiki](http://wiki.telink-semi.cn/)

## 版本历史

| 版本 | 日期 | 说明 |
|------|------|------|
| v1.0 | 2026-05-07 | 初始版本，六路基础PWM输出 |
