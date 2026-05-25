# TLSR8269 六通道脉冲治疗仪 - 编译构建指南

> **版本**：v1.0  
> **日期**：2026-05-24  
> **目标**：指导用户搭建编译环境并生成烧录文件

---

## 一、开发环境要求

### 1.1 硬件要求

| 项目 | 要求 |
|------|------|
| 电脑 | Windows 10/11 或 Linux |
| USB接口 | 至少1个（用于烧录器） |
| 烧录器 | Telink BDT（Burning and Debugging Tool）或兼容工具 |

### 1.2 软件要求

| 软件 | 版本 | 用途 |
|------|------|------|
| Python | 3.8+ | 烧录工具 |
| Git | 任意 | 下载SDK |
| Make | 任意 | 编译 |
| Telink SDK | 最新 | 编译和烧录 |

---

## 二、安装Telink SDK

### 2.1 下载SDK

```bash
# 克隆Telink SDK（TLSR826x系列）
git clone https://github.com/TelinkSemi/telink_826x_sdk.git

# 或者下载压缩包解压
cd telink_826x_sdk
```

### 2.2 安装工具链

#### Windows

1. 下载 [Telink IDE](https://telink-semi.github.io/doc/an/AN0003-Telink-IDE-User-Guide/)
2. 安装到 `C:\Telink\SDK`
3. 添加环境变量：
   ```
   TELINK_SDK_PATH=C:\Telink\SDK
   PATH=%PATH%;%TELINK_SDK_PATH%\tools
   ```

#### Linux

```bash
# 安装依赖
sudo apt-get install gcc make python3 python3-pip

# 安装Telink烧录工具
pip3 install telink-burn

# 设置环境变量
export TELINK_SDK_PATH=/path/to/telink_826x_sdk
export PATH=$PATH:$TELINK_SDK_PATH/tools
```

---

## 三、项目配置

### 3.1 项目结构

```
TLSR8269_SixChannel_Pulse_Generator/
├── firmware/                    # 固件代码
│   ├── main_master_v3.c        # 主程序
│   ├── tlsr8269_reg.h          # 寄存器定义
│   ├── timer_driver.h/c        # 定时器驱动
│   ├── uart_driver.h/c         # UART驱动
│   ├── i2c_driver.h/c          # I2C驱动
│   ├── pwm_driver.h/c          # PWM驱动
│   ├── adc_driver.h/c          # ADC驱动
│   ├── lcd_driver.h/c          # LCD驱动
│   ├── button_driver.h/c       # 按键驱动
│   ├── ui_manager.h/c          # UI管理
│   ├── self_check.c/h          # 自检模块
│   ├── Makefile                # 编译脚本
│   └── BUILD_GUIDE.md          # 本文件
└── ...
```

### 3.2 Makefile

创建 `firmware/Makefile`：

```makefile
# TLSR8269 Pulse Therapy - Makefile

# 项目配置
PROJECT_NAME = PulseTherapy
TARGET = $(PROJECT_NAME)

# SDK路径
TELINK_SDK ?= /path/to/telink_826x_sdk

# 工具链
CC = tc32-elf-gcc
LD = tc32-elf-ld
OBJCOPY = tc32-elf-objcopy
SIZE = tc32-elf-size

# 编译选项
CFLAGS = -ffunction-sections -fdata-sections
CFLAGS += -Wall -O2
CFLAGS += -DCHIP_TYPE=CHIP_TYPE_8269
CFLAGS += -I$(TELINK_SDK)/drivers
CFLAGS += -I$(TELINK_SDK)/common
CFLAGS += -I.

# 链接选项
LDFLAGS = -T$(TELINK_SDK)/boot/link/boot_8269.link
LDFLAGS += --gc-sections

# 源文件
SRCS = main_master_v3.c \
       timer_driver.c \
       uart_driver.c \
       i2c_driver.c \
       pwm_driver.c \
       adc_driver.c \
       lcd_driver.c \
       button_driver.c \
       ui_manager.c \
       self_check.c

# 目标文件
OBJS = $(SRCS:.c=.o)

# 规则
all: $(TARGET).bin
	@echo "Build complete: $(TARGET).bin"
	@echo "Size:"
	@$(SIZE) $(TARGET).elf

$(TARGET).elf: $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $^

$(TARGET).bin: $(TARGET).elf
	$(OBJCOPY) -O binary $< $@

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS) $(TARGET).elf $(TARGET).bin

flash: $(TARGET).bin
	@echo "Flashing to device..."
	python3 $(TELINK_SDK)/tools/bdt/bdt.py flash -p COM3 -b 921600 -i $(TARGET).bin

.PHONY: all clean flash
```

---

## 四、编译步骤

### 4.1 配置环境

```bash
# 进入固件目录
cd TLSR8269_SixChannel_Pulse_Generator/firmware

# 设置SDK路径（根据实际安装位置）
export TELINK_SDK_PATH=/path/to/telink_826x_sdk
```

### 4.2 编译

```bash
# 清理旧文件
make clean

# 编译
make all

# 输出：
# Build complete: PulseTherapy.bin
# Size:
#    text    data     bss     dec     hex filename
#   24576    1024    4096   29696    7400 PulseTherapy.elf
```

### 4.3 常见问题

#### 问题1：找不到编译器

```
make: tc32-elf-gcc: Command not found
```

**解决**：
```bash
# 添加工具链到PATH
export PATH=$PATH:/path/to/tc32/bin

# 或安装Telink IDE
```

#### 问题2：找不到SDK头文件

```
fatal error: drivers/8269/driver_8269.h: No such file
```

**解决**：
```bash
# 检查SDK路径
export TELINK_SDK=/correct/path/to/sdk

# 或修改Makefile中的路径
```

#### 问题3：内存不足

```
region `ram' overflowed by 1234 bytes
```

**解决**：
- 优化代码，减少全局变量
- 启用编译优化 `-Os`
- 减少缓冲区大小

---

## 五、烧录步骤

### 5.1 硬件连接

```
烧录器          TLSR8269模块
──────          ───────────
VCC  ─────────► 3.3V
GND  ─────────► GND
SWM  ─────────► PA7 (SWire)
RST  ─────────► RESET
```

### 5.2 烧录命令

#### 使用Telink BDT工具

```bash
# Windows
bdt.exe flash -p COM3 -b 921600 -i PulseTherapy.bin

# Linux
python3 bdt.py flash -p /dev/ttyUSB0 -b 921600 -i PulseTherapy.bin
```

#### 使用Makefile

```bash
make flash
```

### 5.3 烧录步骤

1. **连接烧录器**到电脑和模块
2. **按住复位键**或短接RESET到GND
3. **执行烧录命令**
4. **释放复位键**
5. **等待烧录完成**

```
[INFO] Connecting to device...
[INFO] Erasing flash...
[INFO] Writing firmware...
[####] 100% (24576/24576 bytes)
[INFO] Verifying...
[INFO] Flash successful!
```

---

## 六、调试方法

### 6.1 UART调试输出

```bash
# 连接USB转串口模块
# TXD ──► PB7 (UART_RX)
# RXD ◄── PB6 (UART_TX)
# GND ──► GND

# 使用串口工具查看日志
# 波特率：115200
# 数据位：8
# 停止位：1
# 校验：无
```

### 6.2 日志级别设置

```c
// 在main()中初始化
log_init(LOG_LEVEL_DEBUG);  // 启用调试日志

// 或使用不同级别
log_init(LOG_LEVEL_ERROR);  // 仅错误
log_init(LOG_LEVEL_INFO);   // 信息+错误
log_init(LOG_LEVEL_VERBOSE);// 全部
```

### 6.3 调试命令

连接串口后，输入命令：

```
help              - 显示帮助
status            - 显示系统状态
pwm 0 10.0 5000   - 设置CH0: 10Hz, 50%占空比
adc 2             - 读取ADC通道2
therapy start     - 开始治疗
therapy stop      - 停止治疗
```

---

## 七、验证测试

### 7.1 上电测试

- [ ] 电源指示灯亮
- [ ] LCD显示启动画面
- [ ] 版本信息正确

### 7.2 自检测试

- [ ] I2C扫描通过
- [ ] PWM测试通过（示波器检查）
- [ ] ADC测试通过
- [ ] 按键测试通过

### 7.3 功能测试

- [ ] 配置界面正常
- [ ] 治疗启动/停止正常
- [ ] 急停响应<10ms
- [ ] 电流限制有效

---

## 八、发布构建

### 8.1 优化编译

```bash
# 使用-Os优化大小
make CFLAGS="-Os -ffunction-sections -fdata-sections"

# 或修改Makefile
```

### 8.2 生成发布文件

```bash
# 编译
make clean
make all

# 复制发布文件
mkdir -p release
cp PulseTherapy.bin release/
cp PulseTherapy.elf release/
cp ../docs/README.md release/
cp ../docs/DISCLAIMER.md release/

# 打包
cd release
zip PulseTherapy_v3.0.zip *
```

---

## 九、参考资源

| 资源 | 链接 |
|------|------|
| Telink SDK | https://github.com/TelinkSemi/telink_826x_sdk |
| Telink文档 | https://telink-semi.github.io/doc/ |
| BDT工具 | SDK目录/tools/bdt/ |
| 社区论坛 | https://github.com/scj009999/TLSR8269_SixChannel_Pulse_Generator |

---

## 十、故障排除

### 10.1 编译错误

| 错误 | 原因 | 解决 |
|------|------|------|
| `undefined reference` | 缺少源文件 | 检查Makefile中的SRCS |
| `multiple definition` | 重复定义 | 检查头文件保护 |
| `segmentation fault` | 栈溢出 | 增加栈大小或减少局部变量 |

### 10.2 烧录错误

| 错误 | 原因 | 解决 |
|------|------|------|
| `Cannot connect` | 连接问题 | 检查接线，按住复位 |
| `Erase failed` | 芯片锁定 | 先执行解锁命令 |
| `Verify failed` | 数据错误 | 重新烧录，检查线长 |

### 10.3 运行错误

| 现象 | 原因 | 解决 |
|------|------|------|
| 无输出 | 程序未运行 | 检查复位电路 |
| 乱码 | 波特率错误 | 匹配波特率设置 |
| 死机 | 看门狗/中断 | 检查中断处理 |

---

*本指南随固件版本更新*
*遇到问题请在GitHub Issues反馈*
