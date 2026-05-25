/**
 * @file i2c_driver.h
 * @brief TLSR8269 I2C驱动 - 硬件I2C Master模式
 * @details 支持SSD1306 OLED和INA219电流检测
 * @author 束长江
 * @version 1.0.0
 * @date 2026-05-24
 * 
 * @note 引脚定义：PB6(SDA), PB7(SCL)
 * @note 医疗级安全：所有操作带超时保护，防止总线死锁
 */

#ifndef __I2C_DRIVER_H__
#define __I2C_DRIVER_H__

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================
 * 引脚定义
 * ============================================================ */
#define I2C_SDA_PIN         14      // PB6 - SDA
#define I2C_SCL_PIN         15      // PB7 - SCL

/* ============================================================
 * 设备地址
 * ============================================================ */
#define I2C_ADDR_SSD1306    0x3C    // OLED显示屏 (0111100x)
#define I2C_ADDR_SSD1306_ALT 0x3D   // OLED备用地址
#define I2C_ADDR_INA219     0x40    // 电流检测芯片 (1000000x)
#define I2C_ADDR_INA219_A1  0x41    // INA219 A1=GND, A0=VS+
#define I2C_ADDR_INA219_A2  0x44    // INA219 A1=VS+, A0=GND
#define I2C_ADDR_INA219_A3  0x45    // INA219 A1=VS+, A0=VS+

/* ============================================================
 * 时钟频率
 * ============================================================ */
typedef enum {
    I2C_FREQ_100K = 100000,     // 标准模式
    I2C_FREQ_400K = 400000,     // 快速模式（默认）
    I2C_FREQ_1M   = 1000000     // 快速模式+
} I2C_FreqTypeDef;

/* ============================================================
 * 错误码
 * ============================================================ */
typedef enum {
    I2C_OK = 0,
    I2C_ERR_BUSY,               // 总线忙
    I2C_ERR_TIMEOUT,            // 超时
    I2C_ERR_NACK,               // 从机无应答
    I2C_ERR_ARB_LOST,           // 仲裁丢失
    I2C_ERR_BUS_ERROR,          // 总线错误
    I2C_ERR_INVALID_PARAM       // 参数错误
} I2C_ErrorTypeDef;

/* ============================================================
 * 状态结构
 * ============================================================ */
typedef struct {
    bool initialized;           // 是否已初始化
    uint32_t freq;              // 当前频率
    uint32_t timeout_ms;        // 超时时间(ms)
    uint32_t error_count;       // 错误计数（医疗级：监控总线健康）
    uint32_t last_error;        // 最后一次错误
    bool bus_recovered;         // 总线是否已恢复
} I2C_StatusTypeDef;

/* ============================================================
 * API函数
 * ============================================================ */

/**
 * @brief 初始化I2C硬件
 * @param freq 时钟频率，推荐 I2C_FREQ_400K
 * @param timeout_ms 操作超时时间，推荐 100ms
 * @return true=成功, false=失败
 * @note 自动配置PB6/PB7为上拉开漏输出
 */
bool i2c_init(I2C_FreqTypeDef freq, uint32_t timeout_ms);

/**
 * @brief 反初始化I2C，释放引脚
 */
void i2c_deinit(void);

/**
 * @brief 检查设备是否在线
 * @param addr 7位设备地址
 * @return true=在线, false=离线
 */
bool i2c_probe(uint8_t addr);

/**
 * @brief 扫描总线上的所有设备
 * @param addr_list 输出地址列表，需预留16字节
 * @return 发现的设备数量
 */
uint8_t i2c_scan(uint8_t *addr_list);

/**
 * @brief 向设备写入数据
 * @param addr 7位设备地址
 * @param data 数据缓冲区
 * @param len 数据长度
 * @return I2C_ErrorTypeDef
 */
I2C_ErrorTypeDef i2c_write(uint8_t addr, const uint8_t *data, uint16_t len);

/**
 * @brief 从设备读取数据
 * @param addr 7位设备地址
 * @param data 数据缓冲区
 * @param len 读取长度
 * @return I2C_ErrorTypeDef
 */
I2C_ErrorTypeDef i2c_read(uint8_t addr, uint8_t *data, uint16_t len);

/**
 * @brief 先写后读（常用操作）
 * @param addr 7位设备地址
 * @param tx_data 发送数据
 * @param tx_len 发送长度
 * @param rx_data 接收缓冲区
 * @param rx_len 接收长度
 * @return I2C_ErrorTypeDef
 */
I2C_ErrorTypeDef i2c_write_read(uint8_t addr, 
                                 const uint8_t *tx_data, uint16_t tx_len,
                                 uint8_t *rx_data, uint16_t rx_len);

/**
 * @brief 向设备寄存器写入单字节
 * @param addr 设备地址
 * @param reg 寄存器地址
 * @param value 写入值
 * @return I2C_ErrorTypeDef
 */
I2C_ErrorTypeDef i2c_write_reg(uint8_t addr, uint8_t reg, uint8_t value);

/**
 * @brief 从设备寄存器读取单字节
 * @param addr 设备地址
 * @param reg 寄存器地址
 * @param value 输出值
 * @return I2C_ErrorTypeDef
 */
I2C_ErrorTypeDef i2c_read_reg(uint8_t addr, uint8_t reg, uint8_t *value);

/**
 * @brief 向设备寄存器写入多字节（16位地址）
 * @param addr 设备地址
 * @param reg 寄存器地址
 * @param data 数据
 * @param len 长度
 * @return I2C_ErrorTypeDef
 */
I2C_ErrorTypeDef i2c_write_reg16(uint8_t addr, uint16_t reg, const uint8_t *data, uint16_t len);

/**
 * @brief 从设备寄存器读取多字节（16位地址）
 * @param addr 设备地址
 * @param reg 寄存器地址
 * @param data 缓冲区
 * @param len 长度
 * @return I2C_ErrorTypeDef
 */
I2C_ErrorTypeDef i2c_read_reg16(uint8_t addr, uint16_t reg, uint8_t *data, uint16_t len);

/**
 * @brief 恢复总线（医疗级：死锁恢复）
 * @details 发送9个时钟脉冲+STOP条件，释放死锁的从机
 * @return true=恢复成功, false=需要硬件复位
 */
bool i2c_bus_recovery(void);

/**
 * @brief 获取I2C状态
 * @return 状态结构体指针
 */
const I2C_StatusTypeDef* i2c_get_status(void);

/**
 * @brief 清除错误计数
 */
void i2c_clear_errors(void);

/**
 * @brief 设置超时时间
 * @param timeout_ms 超时毫秒数
 */
void i2c_set_timeout(uint32_t timeout_ms);

#ifdef __cplusplus
}
#endif

#endif /* __I2C_DRIVER_H__ */
