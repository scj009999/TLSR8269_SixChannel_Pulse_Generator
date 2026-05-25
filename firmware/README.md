# TLSR8269 六通道脉冲发生器 — 固件目录

> **项目**：开源经颅脉冲治疗仪  
> **芯片**：TLSR8269F512ET32 (QFN32)  
> **功能**：六通道PWM脉冲输出，医疗级安全保护

---

## 目录结构

```
firmware/
├── README.md              # 本文件
├── tlsr8269_reg.h         # 芯片寄存器定义
├── self_check.h           # 自检模块头文件
├── self_check.c           # 自检模块实现
├── main.c                 # 主程序（待创建）
├── pwm_driver.c           # PWM驱动（待创建）
├── adc_driver.c           # ADC驱动（待创建）
├── uart_driver.c          # UART驱动（待创建）
├── i2c_driver.c           # I2C驱动（待创建）
├── ble_service.c          # BLE服务（待创建）
└── Makefile               # 编译脚本（待创建）
```

---

## 文件说明

### 1. tlsr8269_reg.h

**功能**：TLSR8269芯片寄存器定义

**包含内容**：
- 时钟控制寄存器
- GPIO寄存器（PA/PB/PC端口）
- PWM寄存器（6通道）
- ADC寄存器（6通道）
- UART/SPI/I2C寄存器
- 中断寄存器
- 电源管理寄存器
- Flash寄存器
- 看门狗寄存器
- 引脚复用功能定义
- 常用宏定义（GPIO操作、PWM使能等）

**使用方法**：
```c
#include "tlsr8269_reg.h"

// 设置PA0为输出
GPIO_SET_OUTPUT(pa, 0);

// 设置PA0输出高电平
GPIO_WRITE(pa, 0, 1);

// 读取PA0输入
uint8_t val = GPIO_READ(pa, 0);
```

### 2. self_check.h / self_check.c

**功能**：医疗级系统自检模块

**自检项目**（8项）：
1. **时钟检测** — 32M系统时钟 + 32K休眠时钟
2. **Flash检测** — 读写测试 + 参数区CRC校验
3. **RAM检测** — Walking bit test + 地址线测试
4. **电源检测** — 电池电压 + 3.3V稳压
5. **ADC检测** — 阻抗检测电路
6. **PWM检测** — 六通道输出测试
7. **急停按钮** — 按钮状态检测
8. **BLE模块** — 蓝牙初始化检测

**使用示例**：
```c
#include "self_check.h"

void main(void) {
    // 初始化硬件
    system_init();
    
    // 执行自检
    uint8_t result = system_self_check();
    
    if (result != CHECK_OK) {
        // 自检失败，锁机报警
        handle_system_error(result);
    }
    
    // 自检通过，进入主循环
    while (1) {
        main_loop();
    }
}
```

**错误码定义**：
```c
#define CHECK_OK            0x00  // 正常
#define CHECK_FAIL_CLK      0x01  // 时钟异常
#define CHECK_FAIL_FLASH    0x02  // 存储异常
#define CHECK_FAIL_RAM      0x03  // 内存异常
#define CHECK_FAIL_BLE      0x04  // 蓝牙异常
#define CHECK_FAIL_ADC      0x05  // ADC异常
#define CHECK_FAIL_PARAM    0x06  // 参数损坏
#define CHECK_FAIL_POWER    0x07  // 电源异常
#define CHECK_FAIL_PWM      0x08  // PWM异常
#define CHECK_FAIL_ESTOP    0x09  // 急停异常
```

---

## 编译说明

### 工具链

- **编译器**：Telink RISC-V GCC 或 ARM GCC
- **烧录器**：Telink BDT (Burning and Debugging Tool)
- **调试器**：JTAG/SWD

### 编译步骤

```bash
# 1. 进入固件目录
cd firmware

# 2. 编译
make

# 3. 烧录
make flash

# 4. 调试
make debug
```

---

## 开发计划

### 已完成 ✅

- [x] 寄存器定义头文件 (`tlsr8269_reg.h`)
- [x] 系统自检模块 (`self_check.h/c`)
- [ ] PWM驱动模块
- [ ] ADC驱动模块
- [ ] UART驱动模块
- [ ] I2C驱动模块
- [ ] BLE服务模块
- [ ] 主程序框架
- [ ] 编译脚本 (Makefile)

---
### 待完成 ⏳
手机APP
## 安全说明

⚠️ **医疗级安全要求**：

1. **开机自检**：必须全部通过才能操作
2. **异常锁机**：任何故障立即关闭输出
3. **急停按钮**：硬件级急停，优先于软件
4. **电流限制**：软件2mA + 硬件保险丝
5. **阻抗检测**：实时监测，异常自动切断
6. **看门狗**：程序跑飞自动复位

---

## 参考文档

- `hardware/TLSR8251_Arduino_Pinout.md` — Arduino引脚映射
- `hardware/PINOUT_CORRECTION.md` — 引脚修正说明
- `docs/MEDICAL_SAFETY.md` — 医疗安全规范

---

*固件开发中，持续更新...*
