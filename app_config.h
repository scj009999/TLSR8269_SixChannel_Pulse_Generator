/**
 * TLSR8269 六路脉冲发生器 - 应用配置文件
 */

#ifndef APP_CONFIG_H_
#define APP_CONFIG_H_

// ============================================================================
// 系统时钟配置
// ============================================================================

// 系统时钟频率 (Hz)
// TLSR8269支持：16MHz, 24MHz, 32MHz, 48MHz
#define CLOCK_SYS_CLOCK_HZ      16000000

// ============================================================================
// BLE/低功耗配置
// ============================================================================

// 禁用BLE功能（纯PWM应用不需要BLE）
#define BLE_MODULE_ENABLE       0

// 禁用低功耗模式（PWM与低功耗模式冲突）
#define BLE_APP_PM_ENABLE       0
#define PM_DEEPSLEEP_RETENTION_ENABLE   0

// ============================================================================
// PWM配置
// ============================================================================

// PWM时钟源选择
// 0: 系统时钟分频
// 1: 外部时钟
#define PWM_CLOCK_SOURCE        0

// PWM默认频率 (Hz)
#define PWM_DEFAULT_FREQ        1000

// PWM默认占空比 (0-10000)
#define PWM_DEFAULT_DUTY        5000    // 50.00%

// ============================================================================
// 调试配置
// ============================================================================

// 使能串口打印调试信息
#define UART_PRINT_DEBUG_ENABLE 1

// 调试串口波特率
#define PRINT_BAUD_RATE         115200

// 调试串口TX引脚 (使用PB1，避免与PWM引脚冲突)
#define DEBUG_INFO_TX_PIN       GPIO_PB1

// ============================================================================
// 引脚复用检查
// ============================================================================

/*
 * TLSR8269 GPIO引脚功能复用表：
 * 
 * 引脚    | 默认功能 | PWM功能    | 其他功能
 * --------|----------|------------|----------
 * PA0     | GPIO     | PWM0       | UART_CTS
 * PA1     | GPIO     | PWM1       | UART_RTS
 * PA2     | GPIO     | PWM2       | I2C_SCL
 * PA3     | GPIO     | PWM3       | I2C_SDA
 * PA4     | GPIO     | PWM4       | SPI_CK
 * PA5     | GPIO     | PWM5       | SPI_CN
 * PA6     | GPIO     | -          | SPI_DI
 * PA7     | GPIO     | -          | SPI_DO
 * PB0     | GPIO     | -          | SWM
 * PB1     | GPIO     | -          | UART_TX
 * PB2     | GPIO     | -          | UART_RX
 * PB3     | GPIO     | -          | -
 * PB4     | GPIO     | -          | -
 * PB5     | GPIO     | -          | -
 * PB6     | GPIO     | -          | -
 * PB7     | GPIO     | -          | -
 * PC0     | GPIO     | -          | -
 * PC1     | GPIO     | -          | -
 * PC2     | GPIO     | -          | -
 * PC3     | GPIO     | -          | -
 * PC4     | GPIO     | -          | - 
 * PD0-PD7 | GPIO     | -          | - 
 * PE0-PE3 | GPIO     | -          | -
 * PF0-PF7 | GPIO     | PWM0-PWM5  | -
 * 
 * 注意：
 * 1. PA0-PA5 均可作为 PWM0-PWM5 输出
 * 2. PF2-PF5 也可作为 PWM0-PWM3 输出
 * 3. 同一PWM通道不能同时在多个引脚输出
 * 4. 本设计使用 PA0-PA5 作为六路PWM输出
 */

#endif /* APP_CONFIG_H_ */
