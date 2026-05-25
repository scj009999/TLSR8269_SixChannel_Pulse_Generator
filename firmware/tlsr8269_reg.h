/**************************************************************************
 * TLSR8269 寄存器定义头文件
 * 基于 Telink TLSR8269 Datasheet 和 Arduino Core
 * 适用：QFN32 封装
 **************************************************************************/

#ifndef __TLSR8269_REG_H__
#define __TLSR8269_REG_H__

#include <stdint.h>

// ============================================
// 基础类型定义
// ============================================
#ifndef BIT
#define BIT(n) (1 << (n))
#endif

// ============================================
// 时钟控制寄存器
// ============================================
#define reg_clk_ctrl        (*(volatile uint32_t *)0x800060)  // 时钟控制寄存器
#define reg_clk_sel         (*(volatile uint32_t *)0x800064)  // 时钟选择寄存器
#define reg_pll_ctrl        (*(volatile uint32_t *)0x800068)  // PLL控制寄存器

// 时钟控制位定义
#define PLL_LOCK_BIT        BIT(2)   // PLL锁定标志
#define CLK_32K_SEL_BIT     BIT(3)   // 32K时钟选择
#define SYS_CLK_DIV_SHIFT   4        // 系统时钟分频移位

// ============================================
// GPIO 寄存器
// ============================================
// PA端口 (GPIO 0-7)
#define reg_pa_in           (*(volatile uint32_t *)0x800580)  // PA输入
#define reg_pa_out          (*(volatile uint32_t *)0x800584)  // PA输出
#define reg_pa_oen          (*(volatile uint32_t *)0x800588)  // PA输出使能
#define reg_pa_pol          (*(volatile uint32_t *)0x80058C)  // PA极性
#define reg_pa_ds           (*(volatile uint32_t *)0x800590)  // PA驱动强度
#define reg_pa_gpio         (*(volatile uint32_t *)0x800594)  // PA GPIO功能
#define reg_pa_ie           (*(volatile uint32_t *)0x800598)  // PA输入使能

// PB端口 (GPIO 8-15)
#define reg_pb_in           (*(volatile uint32_t *)0x8005A0)
#define reg_pb_out          (*(volatile uint32_t *)0x8005A4)
#define reg_pb_oen          (*(volatile uint32_t *)0x8005A8)
#define reg_pb_pol          (*(volatile uint32_t *)0x8005AC)
#define reg_pb_ds           (*(volatile uint32_t *)0x8005B0)
#define reg_pb_gpio         (*(volatile uint32_t *)0x8005B4)
#define reg_pb_ie           (*(volatile uint32_t *)0x8005B8)

// PC端口 (GPIO 16-23)
#define reg_pc_in           (*(volatile uint32_t *)0x8005C0)
#define reg_pc_out          (*(volatile uint32_t *)0x8005C4)
#define reg_pc_oen          (*(volatile uint32_t *)0x8005C8)
#define reg_pc_pol          (*(volatile uint32_t *)0x8005CC)
#define reg_pc_ds           (*(volatile uint32_t *)0x8005D0)
#define reg_pc_gpio         (*(volatile uint32_t *)0x8005D4)
#define reg_pc_ie           (*(volatile uint32_t *)0x8005D8)

// PD端口 (GPIO 24-31) - QFN32不可用
#define reg_pd_in           (*(volatile uint32_t *)0x8005E0)
#define reg_pd_out          (*(volatile uint32_t *)0x8005E4)
#define reg_pd_oen          (*(volatile uint32_t *)0x8005E8)

// ============================================
// PWM 寄存器
// ============================================
#define reg_pwm_clk         (*(volatile uint32_t *)0x800780)  // PWM时钟
#define reg_pwm_ctrl0       (*(volatile uint32_t *)0x800784)  // PWM控制0
#define reg_pwm_ctrl1       (*(volatile uint32_t *)0x800788)  // PWM控制1
#define reg_pwm_ctrl2       (*(volatile uint32_t *)0x80078C)  // PWM控制2
#define reg_pwm_ctrl3       (*(volatile uint32_t *)0x800790)  // PWM控制3
#define reg_pwm_ctrl4       (*(volatile uint32_t *)0x800794)  // PWM控制4
#define reg_pwm_ctrl5       (*(volatile uint32_t *)0x800798)  // PWM控制5

// PWM通道0-5数据
#define reg_pwm0_data       (*(volatile uint32_t *)0x800794)
#define reg_pwm1_data       (*(volatile uint32_t *)0x800795)
#define reg_pwm2_data       (*(volatile uint32_t *)0x800796)
#define reg_pwm3_data       (*(volatile uint32_t *)0x800797)
#define reg_pwm4_data       (*(volatile uint32_t *)0x800798)
#define reg_pwm5_data       (*(volatile uint32_t *)0x800799)

// PWM控制位
#define PWM_ENABLE          BIT(0)
#define PWM_POLARITY        BIT(1)
#define PWM_MODE_MASK       (0x03 << 2)
#define PWM_MODE_SHIFT      2

// ============================================
// ADC 寄存器
// ============================================
#define reg_adc_ctrl        (*(volatile uint32_t *)0x800900)  // ADC控制
#define reg_adc_data        (*(volatile uint32_t *)0x800904)  // ADC数据
#define reg_adc_cfg         (*(volatile uint32_t *)0x800908)  // ADC配置

// ADC通道选择
#define ADC_CHANNEL_0       0x00  // PB0
#define ADC_CHANNEL_1       0x01  // PB1
#define ADC_CHANNEL_2       0x02  // PB2
#define ADC_CHANNEL_3       0x03  // PB3
#define ADC_CHANNEL_4       0x04  // PB4
#define ADC_CHANNEL_5       0x05  // PB5
#define ADC_CHANNEL_VBAT    0x0D  // 电池电压
#define ADC_CHANNEL_TEMP    0x0E  // 温度传感器

// ADC控制位
#define ADC_POWER_ON        BIT(0)
#define ADC_START           BIT(1)
#define ADC_DONE            BIT(2)
#define ADC_CHANNEL_MASK    (0x0F << 4)
#define ADC_CHANNEL_SHIFT   4

// ============================================
// UART 寄存器
// ============================================
#define reg_uart_data       (*(volatile uint32_t *)0x8001C0)  // UART数据
#define reg_uart_ctrl0      (*(volatile uint32_t *)0x8001C4)  // UART控制0
#define reg_uart_ctrl1      (*(volatile uint32_t *)0x8001C8)  // UART控制1
#define reg_uart_ctrl2      (*(volatile uint32_t *)0x8001CC)  // UART控制2
#define reg_uart_status     (*(volatile uint32_t *)0x8001D0)  // UART状态

// UART控制位
#define UART_TX_EN          BIT(0)
#define UART_RX_EN          BIT(1)
#define UART_TX_IRQ_EN      BIT(2)
#define UART_RX_IRQ_EN      BIT(3)
#define UART_PARITY_EN      BIT(4)
#define UART_PARITY_ODD     BIT(5)

// UART状态位
#define UART_TX_DONE        BIT(0)
#define UART_RX_DONE        BIT(1)
#define UART_TX_BUF_EMPTY   BIT(2)
#define UART_RX_BUF_FULL    BIT(3)

// ============================================
// I2C 寄存器
// ============================================
#define reg_i2c_ctrl        (*(volatile uint32_t *)0x800280)  // I2C控制
#define reg_i2c_data        (*(volatile uint32_t *)0x800284)  // I2C数据
#define reg_i2c_status      (*(volatile uint32_t *)0x800288)  // I2C状态

// I2C控制位
#define I2C_ENABLE          BIT(0)
#define I2C_START           BIT(1)
#define I2C_STOP            BIT(2)
#define I2C_READ            BIT(3)
#define I2C_WRITE           BIT(4)
#define I2C_ACK             BIT(5)
#define I2C_NAK             BIT(6)

// ============================================
// SPI 寄存器
// ============================================
#define reg_spi_ctrl        (*(volatile uint32_t *)0x800240)  // SPI控制
#define reg_spi_data        (*(volatile uint32_t *)0x800244)  // SPI数据
#define reg_spi_status      (*(volatile uint32_t *)0x800248)  // SPI状态

// SPI控制位
#define SPI_ENABLE          BIT(0)
#define SPI_MASTER          BIT(1)
#define SPI_CPOL            BIT(2)
#define SPI_CPHA            BIT(3)
#define SPI_LSB_FIRST       BIT(4)

// ============================================
// 中断寄存器
// ============================================
#define reg_irq_mask        (*(volatile uint32_t *)0x800640)  // 中断屏蔽
#define reg_irq_src         (*(volatile uint32_t *)0x800644)  // 中断源
#define reg_irq_src_clr     (*(volatile uint32_t *)0x800648)  // 中断清除

// 中断源位
#define IRQ_TIMER0          BIT(0)
#define IRQ_TIMER1          BIT(1)
#define IRQ_TIMER2          BIT(2)
#define IRQ_UART            BIT(4)
#define IRQ_SPI             BIT(5)
#define IRQ_I2C             BIT(6)
#define IRQ_ADC             BIT(7)
#define IRQ_PWM             BIT(8)
#define IRQ_GPIO            BIT(9)
#define IRQ_BLE             BIT(16)

// ============================================
// 电源管理寄存器
// ============================================
#define reg_pwr_ctrl        (*(volatile uint32_t *)0x80006C)  // 电源控制
#define reg_pwr_mode        (*(volatile uint32_t *)0x800070)  // 电源模式

// 电源模式
#define POWER_MODE_ACTIVE   0x00
#define POWER_MODE_SLEEP    0x01
#define POWER_MODE_DEEP     0x02

// ============================================
// Flash 寄存器
// ============================================
#define reg_flash_ctrl      (*(volatile uint32_t *)0x800000)  // Flash控制
#define reg_flash_addr      (*(volatile uint32_t *)0x800004)  // Flash地址
#define reg_flash_data      (*(volatile uint32_t *)0x800008)  // Flash数据

// Flash控制位
#define FLASH_READ          BIT(0)
#define FLASH_WRITE         BIT(1)
#define FLASH_ERASE         BIT(2)
#define FLASH_BUSY          BIT(3)

// ============================================
// 看门狗寄存器
// ============================================
#define reg_wdog_ctrl       (*(volatile uint32_t *)0x800620)  // 看门狗控制
#define reg_wdog_cnt        (*(volatile uint32_t *)0x800624)  // 看门狗计数

// 看门狗控制位
#define WDOG_ENABLE         BIT(0)
#define WDOG_RESET          BIT(1)
#define WDOG_TIMEOUT_MASK   (0x0F << 2)

// ============================================
// 引脚复用功能定义
// ============================================
// PA端口功能
#define PA_FUNC_GPIO        0
#define PA_FUNC_PWM         1
#define PA_FUNC_SPI         2
#define PA_FUNC_UART        3
#define PA_FUNC_I2C         4

// PB端口功能
#define PB_FUNC_GPIO        0
#define PB_FUNC_PWM         1
#define PB_FUNC_ADC         2
#define PB_FUNC_I2C         3

// PC端口功能
#define PC_FUNC_GPIO        0
#define PC_FUNC_UART        1
#define PC_FUNC_PWM         2

// ============================================
// 引脚驱动强度
// ============================================
#define PIN_DRIVE_2MA       0
#define PIN_DRIVE_4MA       1
#define PIN_DRIVE_8MA       2
#define PIN_DRIVE_12MA      3

// ============================================
// 上拉/下拉配置
// ============================================
#define PIN_PULL_NONE       0
#define PIN_PULL_UP_10K     1
#define PIN_PULL_UP_1M      2
#define PIN_PULL_DOWN_100K  3

// ============================================
// 常用宏定义
// ============================================
// 设置GPIO输出
#define GPIO_SET_OUTPUT(port, pin) \
    do { \
        reg_##port##_oen &= ~BIT(pin); \
        reg_##port##_gpio |= BIT(pin); \
    } while(0)

// 设置GPIO输入
#define GPIO_SET_INPUT(port, pin) \
    do { \
        reg_##port##_oen |= BIT(pin); \
        reg_##port##_gpio |= BIT(pin); \
        reg_##port##_ie |= BIT(pin); \
    } while(0)

// 设置GPIO输出值
#define GPIO_WRITE(port, pin, val) \
    do { \
        if(val) reg_##port##_out |= BIT(pin); \
        else reg_##port##_out &= ~BIT(pin); \
    } while(0)

// 读取GPIO输入值
#define GPIO_READ(port, pin) \
    ((reg_##port##_in >> (pin)) & 0x01)

// 使能PWM功能
#define PWM_ENABLE_PIN(port, pin) \
    do { \
        reg_##port##_gpio &= ~BIT(pin); \
        reg_##port##_oen &= ~BIT(pin); \
    } while(0)

// 使能ADC功能
#define ADC_ENABLE_PIN(pin) \
    do { \
        reg_pb_gpio &= ~BIT(pin); \
        reg_pb_ie |= BIT(pin); \
    } while(0)

#endif /* __TLSR8269_REG_H__ */
