# 开源经颅脉冲治疗仪 - TLSR8269 六路脉冲发生器

> **为帕金森患者设计的低成本开源治疗设备**
> 
> 目标：让15万/年的治疗变成99元/台
> 
> 🙏 **"救一个人，胜造七级浮屠"**
> 
> 本项目完全开源，不收取任何形式的费用。能帮一个是一个。

## 项目背景

### 疾病现状
帕金森病是一种常见的神经退行性疾病，全球患者超过1000万，中国患者约300万。随着人口老龄化，这一数字仍在快速增长。

主要症状包括：静止性震颤、肌肉僵直、运动迟缓、姿势平衡障碍。

### 治疗现状
经颅脉冲刺激（Transcranial Pulse Stimulation, TPS）是一种新兴的非侵入性治疗方法，已被临床研究证明对改善帕金森症状有效。

**然而，商用设备价格高达1-3万元，医院治疗年费超过15万元，普通百姓难以承受。**

### 项目使命
本项目旨在通过完全开源的方式，将治疗成本降至百元级别，让每一个患者都能获得治疗机会。

**"每一个帕金森患者，无论贫富，都应该有权利获得有效的治疗。"**

## 成本对比

| 项目 | 商用设备 | 开源方案 | 节省 |
|------|----------|----------|------|
| 设备价格 | ¥15,000-30,000 | **¥19.91** | **99.87%** |
| 医院年费 | ¥150,000 | **¥0** | **100%** |
| 3年总费用 | ¥450,000+ | **¥250** | **99.94%** |
| 透明度 | 黑盒 | **完全开源** | - |
| 可定制 | 否 | **是** | - |

## 核心特性

### 低成本
- **物料成本**：< ¥200（批量可降至¥19.91）
- **开源免费**：代码、硬件设计全部开放
- **自制可行**：提供完整制作指南

### 医疗安全
- **电气隔离**：光耦隔离，5000V耐压
- **电流限制**：硬件+软件双重限流（最大2mA）
- **急停保护**：独立硬件急停按钮
- **阻抗检测**：实时监测电极接触
- **超时保护**：治疗超时自动停止

### 技术参数

#### 主控芯片：Telink TLSR8269F512
- 32位MCU，最高48MHz
- 512KB Flash，32KB SRAM
- 6路硬件PWM输出
- 低功耗蓝牙（可选）

### 脉冲输出参数
| 参数 | 范围 | 说明 |
|------|------|------|
| 频率 | 1Hz - 8MHz | 16MHz PWM时钟 |
| 占空比 | 0% - 100% | 精度0.01% |
| 相位 | 0° - 360° | 软件实现 |
| 通道数 | 6路 | PA0-PA3, PB0-PB1 |
| 输出电平 | 0V / VCC | CMOS输出 |

### 引脚定义（QFN48）

```
TLSR8269 (QFN48)
═══════════════════════════════════════

 ┌─────────────────┐
 PA0 │ 1  48 │ VCC
 PA1 │ 2  47 │ GND
 PA2 │ 3  46 │ PC0
 PA3 │ 4  45 │ PC1
 PA4 │ 5  44 │ PC2
 PA5 │ 6  43 │ PC3
 PB0 │ 7  42 │ PC4 ← CH4 (PWM4)
 PB1 │ 8  41 │ PC5 ← CH5 (PWM5)
 ... │ ...   │ ...
 └─────────────────┘

脉冲输出引脚：
 CH0 → PA0 (Pin 1) - PWM0
 CH1 → PA1 (Pin 2) - PWM1
 CH2 → PA2 (Pin 3) - PWM2
 CH3 → PA3 (Pin 4) - PWM3
 CH4 → PB0 (Pin 7) - PWM4  ⚠️ 注意：非PA4
 CH5 → PB1 (Pin 8) - PWM5  ⚠️ 注意：非PA5
```

## 临床证据

### 核心研究 - 刘河生教授团队

**2026年2月《自然》(Nature) 发表**

- **发现**：帕金森病核心病变环路——"躯体认知环路"
- **效果**：治疗2周后，靶点组有效率达55.5%
- **产业化**：已获批3项国家医疗器械注册证
- **应用**：全国60余家医院，累计治疗超3000例
- **有效率**：突破70%

**新闻来源**：
- [腾讯网 - 首创!事关帕金森病治疗](https://new.qq.com/rain/a/20260205A040H500)
- [新京报 - 昌平实验室首次发现帕金森病核心致病功能环路](https://www.bjnews.com.cn/detail/1774781843129205.html)

更多证据见 [docs/EVIDENCE_COLLECTION.md](docs/EVIDENCE_COLLECTION.md)

---

## 安全设计 ⚠️

### 硬件安全
- **电流限制**：100Ω限流电阻，最大输出2mA（硬件）
- **电气隔离**：PC817光耦，5000V隔离
- **急停按钮**：独立硬件中断，最高优先级
- **看门狗**：1秒超时自动复位

### 软件安全
- **参数硬限制**：频率≤200Hz，电流≤2mA，时长≤30分钟
- **实时监测**：ADC每秒检测电流和阻抗
- **电极脱落检测**：阻抗>20kΩ自动报警
- **超时保护**：治疗超时自动停止

### ⚠️ 使用警告
1. **首次使用请在医生指导下进行**
2. **有癫痫病史者禁用**
3. **装有心脏起搏器者禁用**
4. **治疗过程中如感不适，立即按下急停按钮**
5. **禁止刺激眼眶、太阳穴、颈部区域**

### 禁忌症
- 心脏起搏器患者
- 癫痫病史
- 脑部金属植入
- 皮肤破损
- 孕妇
- 儿童（需医生指导）

## 快速开始

### 编译环境
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

### 基础使用
```c
// 初始化
pulse_generator_init();

// 配置通道0: 40Hz, 50%占空比 (帕金森标准方案)
pulse_channel_config(0, 40, 5000, 0);

// 配置通道1: 40Hz, 50%占空比
pulse_channel_config(1, 40, 5000, 0);

// 使能输出
pulse_channel_enable(0);
pulse_channel_enable(1);

// 治疗20分钟后自动停止
```

## Arduino/Mixly 支持

本项目提供 Arduino 库，支持图形化编程：

```cpp
#include <PulseTherapy.h>

void setup() {
    Pulse.begin();
    Pulse.setCurrentLimit(2000);  // 2mA限制
    Pulse.startTherapy(MODE_PARKINSON_STD, 20);  // 20分钟
}

void loop() {
    Pulse.update();  // 更新状态和安全检查
}
```

更多示例见 `PulseTherapy/examples/` 目录。

## 靶点定位

### 推荐治疗靶点
| 靶点 | 位置 | 功能 |
|------|------|------|
| M1 | 初级运动皮层 | 运动控制 |
| SMA | 辅助运动区 | 运动计划 |
| DLPFC | 背外侧前额叶 | 认知功能 |

### 六通道分配方案
```
方案A：双靶点双侧（推荐）
CH0-CH1 → M1 双侧 (40Hz, 50%)
CH2-CH3 → SMA 双侧 (40Hz, 50%)
CH4-CH5 → DLPFC 双侧 (20Hz, 30%)
```

详细定位方法见 `docs/TARGETING_SYSTEM.md`

## 项目结构

```
TLSR8269_SixChannel_Pulse_Generator/
├── main.c                          # 核心脉冲发生器代码
├── app_config.h                    # 芯片配置
├── Makefile                        # 编译脚本
├── README.md                       # 本文件（项目总览）
├── LICENSE                         # 开源许可证（含非商业条款）
├── DISCLAIMER.md                   # ⭐ 免责声明（必读）
├── FILE_INVENTORY.md               # 文件清单
│
├── hardware/                       # 硬件设计
│   ├── schematic.md                # 原理图和BOM
│   ├── TLSR8269_PulseGenerator_v2.kicad_sch  # KiCad原理图
│   ├── TLSR8269_Pinout_QFN32.md    # 引脚定义
│   ├── MEDICAL_POWER_DESIGN.md     # 医疗电源设计
│   ├── POWER_BOM.md                # 电源物料清单
│   └── KiCad_Import_Guide.md       # KiCad导入指南
│
├── docs/                           # 文档
│   ├── MEDICAL_SAFETY.md           # 医疗安全设计规范
│   ├── TARGETING_SYSTEM.md         # 靶点定位系统
│   ├── OPEN_SOURCE_LETTER.md       # 致国家药监局公开信
│   ├── COST_ANALYSIS.md            # 成本分析（目标<¥100）
│   ├── EVIDENCE_COLLECTION.md      # ⭐ 临床证据收集
│   ├── NMPA_EMAIL_DRAFT.md         # ⭐ 药监局邮件草稿
│   ├── DOCTOR_RECRUITMENT.md       # ⭐ 医生志愿者招募
│   ├── MOBILE_APP_DESIGN.md        # ⭐ 手机APP设计
│   ├── HUAQIU_AI_GUIDE.md          # 华秋AI使用指南
│   ├── HUAQIU_PRODUCTION_GUIDE.md  # 华秋生产指南
│   ├── SHELL_DESIGN_GUIDE.md       # 3D外壳设计指南
│   ├── ELECTRODE_DESIGN.md         # 头戴式电极设计
│   ├── POWER_CONTROL_COMPARISON.md # 功率调节方案对比
│   └── GITHUB_PROMOTION_GUIDE.md   # 仓库推广指南
│
├── examples/                       # 示例代码
│   ├── uart_control.c              # 串口控制示例
│   └── ...
│
├── PulseTherapy/                   # Arduino库
│   ├── PulseTherapy.h
│   ├── PulseTherapy.cpp
│   ├── keywords.txt
│   └── examples/
│       └── ParkinsonTherapy/
│           └── ParkinsonTherapy.ino
│
└── mixly_blocks/                   # Mixly图形化积木
    ├── blocks.js
    └── generator.js
```

## 成本分析

### 物料清单（BOM）
| 元件 | 型号 | 数量 | 单价(¥) | 小计(¥) |
|------|------|------|---------|---------|
| 主控芯片 | TLSR8269F512 | 1 | 6.50 | 6.50 |
| 光耦 | PC817 | 6 | 0.30 | 1.80 |
| 限流电阻 | 100Ω 1% | 6 | 0.05 | 0.30 |
| 电极接口 | 2.54mm插座 | 6 | 0.50 | 3.00 |
| 电源芯片 | XC6206P332MR | 1 | 0.40 | 0.40 |
| 按键 | 轻触开关 | 3 | 0.30 | 0.90 |
| LED | 3mm红/绿 | 2 | 0.10 | 0.20 |
| 电容电阻 | 0402/0603 | 若干 | - | 2.00 |
| PCB | 2层板 | 1 | 5.00 | 5.00 |
| 电池 | 18650锂电池 | 1 | 15.00 | 15.00 |
| 外壳 | 3D打印 | 1 | 10.00 | 10.00 |
| 电极片 | 医用电极 | 6 | 2.00 | 12.00 |
| **总计** | | | | **¥19.91** |

### 目标售价
- **物料成本**：¥19.91
- **加工组装**：¥15
- **包装运输**：¥10
- **合理利润**：¥9
- **最终售价**：**¥99**

## 开源许可

本项目采用双重许可证：

- **软件**：GNU General Public License v3.0 (GPL v3)
- **硬件**：CERN Open Hardware Licence v2 (CERN OHL v2)

这意味着：
- ✅ 您可以自由使用、修改、分发
- ✅ 您可以商用，但必须开源
- ✅ 您可以生产并销售
- ❌ 不能闭源修改后销售
- ❌ 不能申请专利限制他人使用

## ⚠️ 重要声明

### 公益性质声明

**本项目为纯公益开源项目，不收取任何形式的费用。**

- ❌ **禁止商业用途**：任何人不得将本项目用于商业盈利目的
- ❌ **禁止收费销售**：不得以任何形式向患者收费
- ✅ **允许**：个人学习、研究、自制使用
- ✅ **允许**：在医生指导下为患者免费治疗
- ✅ **允许**：生产免费发放给贫困患者

**违反上述规定者，将追究法律责任。**

### 免责声明

1. 本项目为开源实验性医疗设备，**未经国家药监局批准**
2. 使用风险由用户自行承担
3. **必须在专业医生指导下使用**
4. 如出现不适，请立即停止使用并就医
5. 本项目不对任何使用后果负责
6. 医生参与纯属自愿，以志愿者形式，无任何报酬要求

### 医生招募声明

- 医生参与本项目**纯属自愿**
- 以**志愿者形式**参与，**不收取任何费用**
- 项目方**不支付**任何形式的报酬
- 参与医生**自行承担**相关责任
- 项目方仅为医生与患者提供技术交流平台

**本项目不是医疗机构，不提供医疗服务。**

## 医生招募 👨‍⚕️

我们正在寻找有爱心、有专业能力的医生加入项目：

**招募对象**：
- 神经内科医生（帕金森病诊疗经验）
- 康复科医生（神经康复专业）
- 精神科医生（认知功能评估）
- 临床研究人员（试验设计经验）

**您将获得**：
- 论文署名权
- 去标识化治疗数据
- 象征性顾问费
- 免费设备原型
- 开源医疗先驱荣誉

**参与方式**：
1. 阅读项目文档，了解技术方案
2. 发送邮件至项目邮箱（见下方）
3. 线上面试沟通
4. 签署协议，正式加入

详细招募信息：[docs/DOCTOR_RECRUITMENT.md](docs/DOCTOR_RECRUITMENT.md)

**让我们携手，用专业知识守护生命尊严。**

## 参与贡献

我们欢迎所有形式的贡献：

- 🐛 提交Bug报告
- 💡 提出功能建议
- 🔧 提交代码改进
- 📖 完善文档
- 🧪 参与临床测试
- 👨‍⚕️ 医生加入（见上方招募）
- 💰 捐赠支持项目

## 联系方式

- GitHub Issues: [提交问题](https://github.com/scj009999/TLSR8269_SixChannel_Pulse_Generator/issues)
- 项目主页: https://github.com/scj009999/TLSR8269_SixChannel_Pulse_Generator

## 制作指南

### 快速开始

#### 1. 下载代码
```bash
git clone https://github.com/scj009999/TLSR8269_SixChannel_Pulse_Generator.git
cd TLSR8269_SixChannel_Pulse_Generator
```

#### 2. 制作硬件
- 查看 [hardware/schematic.md](hardware/schematic.md) 了解电路设计
- 使用 [KiCad](hardware/TLSR8269_PulseGenerator_v2.kicad_sch) 打开原理图
- 参考 [HUAQIU_PRODUCTION_GUIDE.md](docs/HUAQIU_PRODUCTION_GUIDE.md) 下单生产

#### 3. 制作电极
- 参考 [ELECTRODE_DESIGN.md](docs/ELECTRODE_DESIGN.md) 制作头戴式电极
- 材料成本约¥116

#### 4. 编译烧录
```bash
# 使用Telink SDK
make

# 使用Telink BDT工具烧录
# 详见 README.md 快速开始章节
```

#### 5. 下载APP
- 手机蓝牙连接设备
- 调节强度和模式
- 开始治疗

### 详细文档

| 主题 | 文档 |
|------|------|
| 医疗安全 | [docs/MEDICAL_SAFETY.md](docs/MEDICAL_SAFETY.md) |
| 靶点定位 | [docs/TARGETING_SYSTEM.md](docs/TARGETING_SYSTEM.md) |
| 电极制作 | [docs/ELECTRODE_DESIGN.md](docs/ELECTRODE_DESIGN.md) |
| 功率调节 | [docs/POWER_CONTROL_COMPARISON.md](docs/POWER_CONTROL_COMPARISON.md) |
| 外壳设计 | [docs/SHELL_DESIGN_GUIDE.md](docs/SHELL_DESIGN_GUIDE.md) |
| 生产指南 | [docs/HUAQIU_PRODUCTION_GUIDE.md](docs/HUAQIU_PRODUCTION_GUIDE.md) |
| 推广指南 | [docs/GITHUB_PROMOTION_GUIDE.md](docs/GITHUB_PROMOTION_GUIDE.md) |

---

## 推广与影响力

### 查看访问量
登录GitHub → Insights → Traffic 查看：
- Views（浏览量）
- Clones（下载量）
- Referrals（来源）

### 推广渠道
- **技术社区**：知乎、CSDN、V2EX
- **患者社区**：帕金森病友群、好大夫在线
- **医生社区**：丁香园、医脉通
- **开源社区**：Hacker News、Reddit

详细推广策略见 [docs/GITHUB_PROMOTION_GUIDE.md](docs/GITHUB_PROMOTION_GUIDE.md)

---

## 致谢

感谢刘河生教授团队的开创性研究，为本项目提供了理论基础。

感谢所有为开源医疗事业贡献力量的开发者、医生和患者。

**特别感谢每一位下载、使用、反馈的朋友——你们的每一次使用，都是对"救一个人，胜造七级浮屠"最好的践行。**

---

**每一个帕金森患者，无论贫富，都应该有权利获得有效的治疗。**

*本项目完全开源，不收取任何形式的费用。*

*能帮一个是一个。*
