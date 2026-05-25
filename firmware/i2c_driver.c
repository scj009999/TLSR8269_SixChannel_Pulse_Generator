/**
 * @file i2c_driver.c
 * @brief TLSR8269 I2C驱动实现 - 硬件I2C Master模式
 * @details 基于Telink SDK I2C硬件模块，支持400kHz快速模式
 * @author 束长江
 * @version 1.0.0
 * @date 2026-05-24
 */

#include "i2c_driver.h"
#include "tlsr8269_reg.h"
#include <string.h>

/* ============================================================
 * TLSR8269 I2C寄存器定义
 * ============================================================ */
#define I2C_BASE_ADDR       0x800080  // I2C模块基地址

// I2C控制寄存器
#define REG_I2C_CTRL        (I2C_BASE_ADDR + 0x00)
#define REG_I2C_STATUS      (I2C_BASE_ADDR + 0x01)
#define REG_I2C_CLK         (I2C_BASE_ADDR + 0x02)
#define REG_I2C_DATA        (I2C_BASE_ADDR + 0x03)
#define REG_I2C_ADDR        (I2C_BASE_ADDR + 0x04)

// I2C控制位
#define I2C_CTRL_EN         (1 << 0)    // 使能I2C
#define I2C_CTRL_MASTER     (1 << 1)    // Master模式
#define I2C_CTRL_START      (1 << 2)    // 发送START
#define I2C_CTRL_STOP       (1 << 3)    // 发送STOP
#define I2C_CTRL_ACK        (1 << 4)    // 发送ACK
#define I2C_CTRL_NACK       (1 << 5)    // 发送NACK
#define I2C_CTRL_IE         (1 << 6)    // 中断使能
#define I2C_CTRL_FLUSH      (1 << 7)    // 刷新FIFO

// I2C状态位
#define I2C_STAT_BUSY       (1 << 0)    // 总线忙
#define I2C_STAT_RX_ACK     (1 << 1)    // 收到ACK
#define I2C_STAT_TX_DONE    (1 << 2)    // 发送完成
#define I2C_STAT_RX_DONE    (1 << 3)    // 接收完成
#define I2C_STAT_ARB_LOST   (1 << 4)    // 仲裁丢失
#define I2C_STAT_BUS_ERR    (1 << 5)    // 总线错误
#define I2C_STAT_TIMEOUT    (1 << 6)    // 超时

/* ============================================================
 * GPIO配置
 * ============================================================ */
#define GPIO_BASE_ADDR      0x800580

#define REG_PB_FUNC         (GPIO_BASE_ADDR + 0x4B)  // PB功能选择
#define REG_PB_OUTPUT       (GPIO_BASE_ADDR + 0x4C)  // PB输出使能
#define REG_PB_INPUT        (GPIO_BASE_ADDR + 0x4D)  // PB输入使能
#define REG_PB_PULLUP       (GPIO_BASE_ADDR + 0x4E)  // PB上拉
#define REG_PB_OEN          (GPIO_BASE_ADDR + 0x4F)  // PB输出控制

// PB6/PB7功能：0=GPIO, 1=I2C
#define PB_FUNC_I2C         ((1 << 6) | (1 << 7))

/* ============================================================
 * 私有变量
 * ============================================================ */
static I2C_StatusTypeDef i2c_status = {0};
static volatile uint32_t i2c_tick = 0;

/* ============================================================
 * 私有函数
 * ============================================================ */

/**
 * @brief 微秒延时（简单实现，实际使用定时器）
 */
static void i2c_delay_us(uint32_t us)
{
    // 48MHz主频，约48个时钟周期1us
    volatile uint32_t count = us * 48;
    while (count--) {
        __asm__("nop");
    }
}

/**
 * @brief 毫秒延时
 */
static void i2c_delay_ms(uint32_t ms)
{
    for (uint32_t i = 0; i < ms; i++) {
        i2c_delay_us(1000);
    }
}

/**
 * @brief 等待状态位，带超时
 */
static bool i2c_wait_status(uint8_t mask, uint32_t timeout_ms)
{
    uint32_t start = i2c_tick;
    while ((read_reg8(REG_I2C_STATUS) & mask) == 0) {
        if ((i2c_tick - start) > timeout_ms) {
            return false;  // 超时
        }
    }
    return true;
}

/**
 * @brief 等待总线空闲
 */
static bool i2c_wait_idle(uint32_t timeout_ms)
{
    uint32_t start = i2c_tick;
    while (read_reg8(REG_I2C_STATUS) & I2C_STAT_BUSY) {
        if ((i2c_tick - start) > timeout_ms) {
            return false;
        }
    }
    return true;
}

/**
 * @brief 发送START条件
 */
static I2C_ErrorTypeDef i2c_send_start(void)
{
    if (!i2c_wait_idle(i2c_status.timeout_ms)) {
        i2c_status.last_error = I2C_ERR_BUSY;
        i2c_status.error_count++;
        return I2C_ERR_BUSY;
    }
    
    write_reg8(REG_I2C_CTRL, I2C_CTRL_EN | I2C_CTRL_MASTER | I2C_CTRL_START);
    
    if (!i2c_wait_status(I2C_STAT_TX_DONE, i2c_status.timeout_ms)) {
        i2c_status.last_error = I2C_ERR_TIMEOUT;
        i2c_status.error_count++;
        return I2C_ERR_TIMEOUT;
    }
    
    return I2C_OK;
}

/**
 * @brief 发送STOP条件
 */
static void i2c_send_stop(void)
{
    write_reg8(REG_I2C_CTRL, I2C_CTRL_EN | I2C_CTRL_MASTER | I2C_CTRL_STOP);
    i2c_wait_idle(i2c_status.timeout_ms);
}

/**
 * @brief 发送字节并检查ACK
 */
static I2C_ErrorTypeDef i2c_send_byte(uint8_t data)
{
    write_reg8(REG_I2C_DATA, data);
    write_reg8(REG_I2C_CTRL, I2C_CTRL_EN | I2C_CTRL_MASTER);
    
    if (!i2c_wait_status(I2C_STAT_TX_DONE, i2c_status.timeout_ms)) {
        i2c_status.last_error = I2C_ERR_TIMEOUT;
        i2c_status.error_count++;
        i2c_send_stop();
        return I2C_ERR_TIMEOUT;
    }
    
    if (!(read_reg8(REG_I2C_STATUS) & I2C_STAT_RX_ACK)) {
        i2c_status.last_error = I2C_ERR_NACK;
        i2c_status.error_count++;
        i2c_send_stop();
        return I2C_ERR_NACK;
    }
    
    return I2C_OK;
}

/**
 * @brief 接收字节
 */
static uint8_t i2c_recv_byte(bool ack)
{
    uint8_t ctrl = I2C_CTRL_EN | I2C_CTRL_MASTER;
    if (ack) {
        ctrl |= I2C_CTRL_ACK;
    } else {
        ctrl |= I2C_CTRL_NACK;
    }
    
    write_reg8(REG_I2C_CTRL, ctrl);
    
    i2c_wait_status(I2C_STAT_RX_DONE, i2c_status.timeout_ms);
    
    return read_reg8(REG_I2C_DATA);
}

/* ============================================================
 * API实现
 * ============================================================ */

bool i2c_init(I2C_FreqTypeDef freq, uint32_t timeout_ms)
{
    if (i2c_status.initialized) {
        return true;  // 已初始化
    }
    
    // 保存参数
    i2c_status.freq = freq;
    i2c_status.timeout_ms = timeout_ms;
    i2c_status.error_count = 0;
    i2c_status.last_error = I2C_OK;
    i2c_status.bus_recovered = true;
    
    // 配置GPIO：PB6/PB7为I2C功能
    // 先设为GPIO开漏输出
    uint8_t pb_oen = read_reg8(REG_PB_OEN);
    pb_oen &= ~((1 << 6) | (1 << 7));  // 设为输入（开漏需要）
    write_reg8(REG_PB_OEN, pb_oen);
    
    // 使能上拉
    uint8_t pb_pullup = read_reg8(REG_PB_PULLUP);
    pb_pullup |= (1 << 6) | (1 << 7);
    write_reg8(REG_PB_PULLUP, pb_pullup);
    
    // 切换到I2C功能
    uint8_t pb_func = read_reg8(REG_PB_FUNC);
    pb_func |= PB_FUNC_I2C;
    write_reg8(REG_PB_FUNC, pb_func);
    
    // 计算时钟分频：I2C时钟 = 系统时钟 / (2 * (div + 1))
    // 系统时钟48MHz，目标400kHz：div = 48MHz/(2*400kHz) - 1 = 59
    uint8_t div;
    switch (freq) {
        case I2C_FREQ_100K:
            div = 239;  // 48MHz/(2*240) = 100kHz
            break;
        case I2C_FREQ_400K:
            div = 59;   // 48MHz/(2*60) = 400kHz
            break;
        case I2C_FREQ_1M:
            div = 23;   // 48MHz/(2*24) = 1MHz
            break;
        default:
            div = 59;
    }
    
    write_reg8(REG_I2C_CLK, div);
    
    // 使能I2C Master
    write_reg8(REG_I2C_CTRL, I2C_CTRL_EN | I2C_CTRL_MASTER | I2C_CTRL_FLUSH);
    
    i2c_status.initialized = true;
    
    // 尝试恢复总线（防止上电时从机死锁）
    i2c_bus_recovery();
    
    return true;
}

void i2c_deinit(void)
{
    if (!i2c_status.initialized) {
        return;
    }
    
    // 发送STOP
    i2c_send_stop();
    
    // 禁用I2C
    write_reg8(REG_I2C_CTRL, 0);
    
    // 恢复GPIO功能
    uint8_t pb_func = read_reg8(REG_PB_FUNC);
    pb_func &= ~PB_FUNC_I2C;
    write_reg8(REG_PB_FUNC, pb_func);
    
    memset(&i2c_status, 0, sizeof(i2c_status));
}

bool i2c_probe(uint8_t addr)
{
    if (!i2c_status.initialized) {
        return false;
    }
    
    I2C_ErrorTypeDef err = i2c_send_start();
    if (err != I2C_OK) {
        return false;
    }
    
    err = i2c_send_byte((addr << 1) | 0);  // 写地址
    
    i2c_send_stop();
    
    return (err == I2C_OK);
}

uint8_t i2c_scan(uint8_t *addr_list)
{
    if (!i2c_status.initialized || addr_list == NULL) {
        return 0;
    }
    
    uint8_t count = 0;
    
    for (uint8_t addr = 0x08; addr < 0x78; addr++) {
        if (i2c_probe(addr)) {
            addr_list[count++] = addr;
            if (count >= 16) {
                break;  // 缓冲区满
            }
        }
        i2c_delay_ms(1);  // 设备间延时
    }
    
    return count;
}

I2C_ErrorTypeDef i2c_write(uint8_t addr, const uint8_t *data, uint16_t len)
{
    if (!i2c_status.initialized || data == NULL || len == 0) {
        return I2C_ERR_INVALID_PARAM;
    }
    
    I2C_ErrorTypeDef err;
    
    // START
    err = i2c_send_start();
    if (err != I2C_OK) return err;
    
    // 发送地址（写）
    err = i2c_send_byte((addr << 1) | 0);
    if (err != I2C_OK) return err;
    
    // 发送数据
    for (uint16_t i = 0; i < len; i++) {
        err = i2c_send_byte(data[i]);
        if (err != I2C_OK) return err;
    }
    
    // STOP
    i2c_send_stop();
    
    return I2C_OK;
}

I2C_ErrorTypeDef i2c_read(uint8_t addr, uint8_t *data, uint16_t len)
{
    if (!i2c_status.initialized || data == NULL || len == 0) {
        return I2C_ERR_INVALID_PARAM;
    }
    
    I2C_ErrorTypeDef err;
    
    // START
    err = i2c_send_start();
    if (err != I2C_OK) return err;
    
    // 发送地址（读）
    err = i2c_send_byte((addr << 1) | 1);
    if (err != I2C_OK) return err;
    
    // 接收数据
    for (uint16_t i = 0; i < len; i++) {
        bool ack = (i < len - 1);  // 最后一个字节发NACK
        data[i] = i2c_recv_byte(ack);
    }
    
    // STOP
    i2c_send_stop();
    
    return I2C_OK;
}

I2C_ErrorTypeDef i2c_write_read(uint8_t addr, 
                                 const uint8_t *tx_data, uint16_t tx_len,
                                 uint8_t *rx_data, uint16_t rx_len)
{
    if (!i2c_status.initialized || tx_data == NULL || rx_data == NULL) {
        return I2C_ERR_INVALID_PARAM;
    }
    
    I2C_ErrorTypeDef err;
    
    // START
    err = i2c_send_start();
    if (err != I2C_OK) return err;
    
    // 发送地址（写）
    err = i2c_send_byte((addr << 1) | 0);
    if (err != I2C_OK) return err;
    
    // 发送数据
    for (uint16_t i = 0; i < tx_len; i++) {
        err = i2c_send_byte(tx_data[i]);
        if (err != I2C_OK) return err;
    }
    
    // Repeated START
    err = i2c_send_start();
    if (err != I2C_OK) return err;
    
    // 发送地址（读）
    err = i2c_send_byte((addr << 1) | 1);
    if (err != I2C_OK) return err;
    
    // 接收数据
    for (uint16_t i = 0; i < rx_len; i++) {
        bool ack = (i < rx_len - 1);
        rx_data[i] = i2c_recv_byte(ack);
    }
    
    // STOP
    i2c_send_stop();
    
    return I2C_OK;
}

I2C_ErrorTypeDef i2c_write_reg(uint8_t addr, uint8_t reg, uint8_t value)
{
    uint8_t data[2] = {reg, value};
    return i2c_write(addr, data, 2);
}

I2C_ErrorTypeDef i2c_read_reg(uint8_t addr, uint8_t reg, uint8_t *value)
{
    return i2c_write_read(addr, &reg, 1, value, 1);
}

I2C_ErrorTypeDef i2c_write_reg16(uint8_t addr, uint16_t reg, const uint8_t *data, uint16_t len)
{
    if (data == NULL || len == 0) {
        return I2C_ERR_INVALID_PARAM;
    }
    
    uint8_t tx_buf[32];  // 临时缓冲区
    if (len + 2 > sizeof(tx_buf)) {
        return I2C_ERR_INVALID_PARAM;
    }
    
    tx_buf[0] = (reg >> 8) & 0xFF;  // 高字节
    tx_buf[1] = reg & 0xFF;          // 低字节
    memcpy(&tx_buf[2], data, len);
    
    return i2c_write(addr, tx_buf, len + 2);
}

I2C_ErrorTypeDef i2c_read_reg16(uint8_t addr, uint16_t reg, uint8_t *data, uint16_t len)
{
    if (data == NULL) {
        return I2C_ERR_INVALID_PARAM;
    }
    
    uint8_t tx_buf[2];
    tx_buf[0] = (reg >> 8) & 0xFF;
    tx_buf[1] = reg & 0xFF;
    
    return i2c_write_read(addr, tx_buf, 2, data, len);
}

bool i2c_bus_recovery(void)
{
    // 医疗级：总线死锁恢复
    // 发送9个时钟脉冲+STOP条件
    
    // 临时切回GPIO模式
    uint8_t pb_func = read_reg8(REG_PB_FUNC);
    write_reg8(REG_PB_FUNC, pb_func & ~PB_FUNC_I2C);
    
    // 配置为开漏输出
    uint8_t pb_oen = read_reg8(REG_PB_OEN);
    pb_oen |= (1 << 6) | (1 << 7);  // 输出模式
    write_reg8(REG_PB_OEN, pb_oen);
    
    uint8_t pb_output = read_reg8(REG_PB_OUTPUT);
    
    // 发送9个时钟脉冲
    for (int i = 0; i < 9; i++) {
        // SCL低
        pb_output &= ~(1 << 7);
        write_reg8(REG_PB_OUTPUT, pb_output);
        i2c_delay_us(5);
        
        // SCL高
        pb_output |= (1 << 7);
        write_reg8(REG_PB_OUTPUT, pb_output);
        i2c_delay_us(5);
    }
    
    // STOP条件：SDA从低到高，SCL高
    pb_output &= ~(1 << 6);  // SDA低
    write_reg8(REG_PB_OUTPUT, pb_output);
    i2c_delay_us(5);
    
    pb_output |= (1 << 7);   // SCL高
    write_reg8(REG_PB_OUTPUT, pb_output);
    i2c_delay_us(5);
    
    pb_output |= (1 << 6);   // SDA高
    write_reg8(REG_PB_OUTPUT, pb_output);
    i2c_delay_us(5);
    
    // 恢复I2C功能
    write_reg8(REG_PB_FUNC, pb_func);
    
    // 检查总线是否恢复
    i2c_delay_ms(1);
    
    uint8_t status = read_reg8(REG_I2C_STATUS);
    if (status & I2C_STAT_BUS_ERR) {
        i2c_status.bus_recovered = false;
        i2c_status.error_count++;
        return false;
    }
    
    i2c_status.bus_recovered = true;
    return true;
}

const I2C_StatusTypeDef* i2c_get_status(void)
{
    return &i2c_status;
}

void i2c_clear_errors(void)
{
    i2c_status.error_count = 0;
    i2c_status.last_error = I2C_OK;
}

void i2c_set_timeout(uint32_t timeout_ms)
{
    i2c_status.timeout_ms = timeout_ms;
}

/* ============================================================
 * 定时器回调（由系统定时器调用）
 * ============================================================ */
void i2c_tick_handler(void)
{
    i2c_tick++;
}
