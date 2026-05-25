# TLSR8269 驱动模块开发指南

> **版本**：v2.0  
> **日期**：2026-05-24  
> **分类**：固件驱动开发

---

## 一、驱动模块清单

| 模块 | 文件 | 状态 | 说明 |
|------|------|------|------|
| LCD显示 | `lcd_driver.h/c` | ✅ 已完成 | SSD1306 OLED，128x64，I2C |
| 按键输入 | `button_driver.h/c` | ✅ 已完成 | 8按键，消抖/长按/双击 |
| 界面管理 | `ui_manager.h/c` | ✅ 已完成 | 菜单系统，多界面切换 |
| I2C通信 | `i2c_driver.h/c` | ✅ 已完成 | INA219、OLED等 |
| PWM输出 | `pwm_driver.h/c` | ✅ 已完成 | 6通道PWM |
| ADC采样 | `adc_driver.h/c` | ✅ 已完成 | 阻抗/电流检测 |
| UART通信 | `uart_driver.h/c` | ⏳ 待实现 | 调试/蓝牙透传 |
| BLE蓝牙 | `ble_driver.h/c` | ⏳ 待实现 | 主从通信 |
| 定时器 | `timer_driver.h/c` | ⏳ 待实现 | 系统时钟 |

---

## 二、LCD显示驱动 (`lcd_driver.h/c`)

### 2.1 功能特性

- **分辨率**：128x64 像素
- **接口**：I2C (400kHz)
- **控制器**：SSD1306
- **显存**：1KB (128x64/8)
- **字体**：6x8、8x16 ASCII
- **帧率**：最高60fps

### 2.2 引脚定义

| 信号 | 引脚 | 说明 |
|------|------|------|
| SCL | PB0 (Pin 8) | I2C时钟 |
| SDA | PB3 (Pin 11) | I2C数据 |

### 2.3 使用示例

```c
#include "lcd_driver.h"

void main(void)
{
    // 初始化LCD
    if (!lcd_init()) {
        uart_send_string("LCD init failed!\r\n");
        return;
    }
    
    // 清屏
    lcd_clear();
    
    // 显示标题
    lcd_draw_title("Pulse Therapy");
    
    // 显示字符串
    lcd_draw_string(0, 16, "Channel 0: ON", true);
    lcd_draw_string(0, 24, "Freq: 10.0 Hz", true);
    
    // 显示进度条
    lcd_draw_progress_bar(0, 40, 100, 50);  // 50%
    
    // 刷新显示
    lcd_refresh();
}
```

### 2.4 API函数

| 函数 | 说明 |
|------|------|
| `lcd_init()` | 初始化LCD |
| `lcd_clear()` | 清屏 |
| `lcd_refresh()` | 刷新显示 |
| `lcd_draw_pixel(x,y,color)` | 画点 |
| `lcd_draw_string(x,y,str,color)` | 显示字符串 |
| `lcd_draw_number(x,y,num,color)` | 显示整数 |
| `lcd_draw_float(x,y,num,decimals,color)` | 显示浮点数 |
| `lcd_draw_progress_bar(x,y,w,percent)` | 进度条 |
| `lcd_draw_channel_status(ch,...)` | 通道状态 |

---

## 三、按键驱动 (`button_driver.h/c`)

### 3.1 功能特性

- **按键数**：最多8个
- **消抖时间**：20ms（可配置）
- **长按时间**：1秒（可配置）
- **双击间隔**：300ms（可配置）
- **扫描频率**：100Hz

### 3.2 引脚定义

| 按键 | 引脚 | 功能 |
|------|------|------|
| UP | PC0 (Pin 16) | 上 |
| DOWN | PC1 (Pin 17) | 下 |
| LEFT | PC2 (Pin 18) | 左 |
| RIGHT | PC3 (Pin 19) | 右 |
| OK | PC4 (Pin 20) | 确认 |
| CANCEL | PC5 (Pin 21) | 取消 |
| START | PD0 (Pin 24) | 启动 |
| STOP | PD1 (Pin 25) | 停止/急停 |

### 3.3 使用示例

```c
#include "button_driver.h"

// 按键回调函数
void my_button_handler(uint8_t id, Button_EventTypeDef event)
{
    switch (id) {
        case BTN_OK:
            if (event == BUTTON_EVENT_SHORT_PRESS) {
                uart_send_string("OK pressed\r\n");
            }
            break;
        case BTN_START:
            if (event == BUTTON_EVENT_SHORT_PRESS) {
                start_therapy();
            } else if (event == BUTTON_EVENT_LONG_PRESS) {
                emergency_stop();
            }
            break;
    }
}

void main(void)
{
    // 初始化按键
    button_init();
    button_init_default_layout();
    
    // 注册回调
    button_set_callback(my_button_handler);
    
    // 主循环
    while (1) {
        button_scan();  // 每10ms扫描一次
        delay_ms(10);
    }
}
```

### 3.4 事件类型

| 事件 | 说明 | 触发条件 |
|------|------|----------|
| `PRESS` | 按下 | 消抖确认 |
| `RELEASE` | 释放 | 按键松开 |
| `SHORT_PRESS` | 短按 | < 1秒 |
| `LONG_PRESS` | 长按 | > 1秒 |
| `DOUBLE_CLICK` | 双击 | 300ms内两次 |
| `HOLD` | 持续按住 | 每500ms触发 |

---

## 四、界面管理 (`ui_manager.h/c`)

### 4.1 功能特性

- **多界面切换**：主界面、菜单、编辑、运行等
- **自动刷新**：10fps限制，降低CPU占用
- **事件驱动**：按键事件自动分发
- **数据绑定**：通道状态自动更新

### 4.2 界面模式

| 模式 | 说明 |
|------|------|
| `UI_MODE_MAIN` | 主界面，显示通道概览 |
| `UI_MODE_MENU` | 主菜单 |
| `UI_MODE_CHANNEL_EDIT` | 通道参数编辑 |
| `UI_MODE_SCHEME_SELECT` | 方案选择 |
| `UI_MODE_RUNNING` | 治疗中界面 |
| `UI_MODE_FAULT` | 故障显示 |
| `UI_MODE_SETTINGS` | 设置界面 |

### 4.3 使用示例

```c
#include "ui_manager.h"

void main(void)
{
    // 初始化UI（包含LCD和按键）
    if (!ui_init()) {
        uart_send_string("UI init failed!\r\n");
        return;
    }
    
    // 更新通道状态
    ui_update_channel_status(0, true, 10.0f, 50);
    ui_update_channel_status(1, true, 10.0f, 50);
    
    // 主循环
    while (1) {
        ui_process();  // 处理按键+刷新显示
    }
}
```

### 4.4 界面截图

**主界面**：
```
+--------------------------+
|     Pulse Therapy        |
+--------------------------+
| Channels:                |
| CH0:10.0Hz  50% ████     |
| CH1:10.0Hz  50% ████     |
| CH2: OFF                 |
| CH3: OFF                 |
| CH4: OFF                 |
| CH5: OFF                 |
| [OK]Menu [Start]Run      |
+--------------------------+
```

**运行中界面**：
```
+--------------------------+
|    Therapy Running       |
+--------------------------+
| [████████░░░░░░░░░░]     |
|    05:23 / 20:00         |
| Active: C0 C1            |
|                          |
|                          |
| [Stop]Stop Therapy       |
+--------------------------+
```

---

## 五、待实现驱动

### 5.1 I2C驱动 (`i2c_driver.h/c`) ✅

**状态**：已完成  
**大小**：4,937 + 12,984 = 17,921 bytes  
**引脚**：PB6(SDA), PB7(SCL)  
**频率**：100K/400K/1M Hz

```c
bool i2c_init(I2C_FreqTypeDef freq, uint32_t timeout_ms);
void i2c_deinit(void);
bool i2c_probe(uint8_t addr);
uint8_t i2c_scan(uint8_t *addr_list);
I2C_ErrorTypeDef i2c_write(uint8_t addr, const uint8_t *data, uint16_t len);
I2C_ErrorTypeDef i2c_read(uint8_t addr, uint8_t *data, uint16_t len);
I2C_ErrorTypeDef i2c_write_read(uint8_t addr, const uint8_t *tx_data, uint16_t tx_len, uint8_t *rx_data, uint16_t rx_len);
I2C_ErrorTypeDef i2c_write_reg(uint8_t addr, uint8_t reg, uint8_t value);
I2C_ErrorTypeDef i2c_read_reg(uint8_t addr, uint8_t reg, uint8_t *value);
bool i2c_bus_recovery(void);  // 医疗级：死锁恢复
```

**医疗级特性**：
- 超时保护（可配置，默认100ms）
- 总线死锁自动恢复（9时钟脉冲+STOP）
- 错误计数监控
- 设备扫描和探测

### 5.2 PWM驱动 (`pwm_driver.h/c`) ✅

**状态**：已完成  
**大小**：7,572 + 16,821 = 24,393 bytes  
**引脚**：PA0-PA3(PWM0-3), PB0-PB1(PWM4-5)  
**频率**：1Hz - 8MHz  
**占空比精度**：0.01%

```c
bool pwm_init(void);
void pwm_deinit(void);
bool pwm_config_channel(uint8_t ch, const PWM_ChannelConfigTypeDef *config);
bool pwm_set_frequency(uint8_t ch, float freq_hz);
bool pwm_set_duty(uint8_t ch, uint16_t duty);  // 0-10000
bool pwm_start_channel(uint8_t ch);
void pwm_stop_channel(uint8_t ch);
void pwm_start_all(void);
void pwm_stop_all(void);
void pwm_emergency_stop(void);  // 医疗级：<10ms响应
void pwm_clear_emergency(void);
float pwm_read_current(uint8_t ch);
uint16_t pwm_read_impedance(uint8_t ch);
bool pwm_check_electrode(uint8_t ch);
```

**医疗级特性**：
- 急停响应 <10ms（直接写寄存器）
- 硬件电流限制 20mA
- 阻抗检测（500Ω-10kΩ）
- 故障类型分类（过流/开路/短路/过压）
- 脉冲计数和回调

### 5.3 ADC驱动 (`adc_driver.h/c`) ✅

**状态**：已完成  
**大小**：8,077 + 16,374 = 24,451 bytes  
**引脚**：PB0-PB5（ADC0-5）  
**分辨率**：12位（过采样支持14/16位）  
**精度**：±1LSB

```c
bool adc_init(void);
void adc_deinit(void);
bool adc_config_channel(uint8_t ch, const ADC_ChannelConfigTypeDef *config);
uint16_t adc_read_raw(uint8_t ch);
float adc_read_voltage(uint8_t ch);
ADC_SampleTypeDef adc_sample(uint8_t ch);  // 带过采样和滤波
uint16_t adc_read_impedance(uint8_t ch);   // 分压法
float adc_read_current(uint8_t ch);        // 分流电阻/INA219
float adc_read_output_voltage(uint8_t ch);
uint16_t adc_read_potentiometer(uint8_t ch);  // 0-10000
bool adc_check_electrode(uint8_t ch);
bool adc_calibrate(void);  // 零点校准
bool adc_start_scan(const uint8_t *channels, uint8_t count, uint16_t interval_ms);
void adc_scan_handler(void);  // 主循环调用
```

**医疗级特性**：
- 16次过采样（提高精度）
- 零点校准
- 安全阈值回调（过流/开路/短路）
- 连续扫描模式
- 阻抗范围检测：500Ω-10kΩ

### 5.4 UART驱动 (`uart_driver.h/c`)

```c
// 需要实现：
bool uart_init(uint32_t baudrate);
void uart_send_byte(uint8_t data);
void uart_send_string(const char *str);
uint16_t uart_recv(uint8_t *buf, uint16_t len);
```

### 5.5 BLE驱动 (`ble_driver.h/c`)

```c
// 需要实现：
bool ble_init(void);
bool ble_scan(uint8_t *addr, uint16_t timeout_ms);
bool ble_connect(uint8_t *addr);
bool ble_disconnect(void);
bool ble_send(uint8_t *addr, uint8_t *data, uint16_t len);
uint16_t ble_recv(uint8_t *buf, uint16_t len);
```

### 5.6 定时器驱动 (`timer_driver.h/c`)

```c
// 需要实现：
void timer_init(void);
uint32_t get_system_tick(void);  // 获取系统tick（ms）
void delay_ms(uint16_t ms);
void delay_us(uint16_t us);
```

---

## 六、驱动依赖关系

```
ui_manager.c
    ├── lcd_driver.c
    │       └── i2c_driver.c (待实现)
    ├── button_driver.c
    │       └── timer_driver.c (待实现)
    └── main_master.c
            ├── ble_driver.c (待实现)
            ├── pwm_driver.c (待实现)
            ├── adc_driver.c (待实现)
            └── uart_driver.c (待实现)
```

---

## 七、下一步开发计划

### 优先级1（必须）✅ 已完成
- [x] I2C驱动 — LCD和INA219依赖 (17,921 bytes)
- [x] PWM驱动 — 核心功能 (24,393 bytes)
- [x] ADC驱动 — 安全检测 (24,451 bytes)

### 优先级2（重要）
- [ ] UART驱动 — 调试和日志
- [ ] 定时器驱动 — 系统时钟

### 优先级3（可选）
- [ ] BLE驱动 — 无线通信
- [ ] Flash驱动 — 参数存储

---

*驱动开发需参考TLSR8269官方SDK和Datasheet*
