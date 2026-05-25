/**
 * @file flash_driver.c
 * @brief TLSR8269 Flash存储驱动实现
 * @details 基于Telink SDK Flash API
 */

#include "flash_driver.h"
#include "timer_driver.h"
#include <string.h>

/* ============================================================
 * 寄存器定义（基于Telink TLSR8269）
 * ============================================================ */

// Flash控制寄存器
#define REG_FLASH_CTRL          0x0D   // Flash控制
#define REG_FLASH_ADDR          0x0E   // Flash地址（低16位）
#define REG_FLASH_ADDR_HIGH     0x0F   // Flash地址（高8位）
#define REG_FLASH_DATA          0x10   // Flash数据
#define REG_FLASH_STATUS        0x11   // Flash状态

// Flash命令
#define FLASH_CMD_READ          0x03   // 读取
#define FLASH_CMD_FAST_READ     0x0B   // 快速读取
#define FLASH_CMD_PAGE_PROG     0x02   // 页编程
#define FLASH_CMD_SECTOR_ERASE  0x20   // 扇区擦除(4KB)
#define FLASH_CMD_BLOCK_ERASE   0x52   // 块擦除(32KB)
#define FLASH_CMD_CHIP_ERASE    0x60   // 芯片擦除
#define FLASH_CMD_WRITE_EN      0x06   // 写使能
#define FLASH_CMD_WRITE_DIS     0x04   // 写禁止
#define FLASH_CMD_READ_STATUS   0x05   // 读状态
#define FLASH_CMD_READ_ID       0x9F   // 读ID

// Flash状态位
#define FLASH_STATUS_BUSY       0x01   // 忙
#define FLASH_STATUS_WEL        0x02   // 写使能锁存

/* ============================================================
 * 内部变量
 * ============================================================ */
static bool flash_initialized = false;
static uint32_t flash_capacity = 0;
static uint16_t log_write_index = 0;
static uint16_t log_count = 0;

/* ============================================================
 * 内联读写函数
 * ============================================================ */
static inline void flash_write_reg8(uint16_t addr, uint8_t val) {
    *((volatile uint8_t *)addr) = val;
}

static inline uint8_t flash_read_reg8(uint16_t addr) {
    return *((volatile uint8_t *)addr);
}

static inline void flash_write_reg16(uint16_t addr, uint16_t val) {
    *((volatile uint16_t *)addr) = val;
}

static inline uint16_t flash_read_reg16(uint16_t addr) {
    return *((volatile uint16_t *)addr);
}

/* ============================================================
 * 内部函数
 * ============================================================ */

/**
 * @brief 等待Flash就绪
 */
static void flash_wait_ready(void)
{
    while (flash_read_reg8(REG_FLASH_STATUS) & FLASH_STATUS_BUSY) {
        // 等待忙标志清除
    }
}

/**
 * @brief 发送命令
 */
static void flash_send_cmd(uint8_t cmd)
{
    flash_write_reg8(REG_FLASH_CTRL, cmd);
}

/**
 * @brief 设置地址
 */
static void flash_set_addr(uint32_t addr)
{
    flash_write_reg16(REG_FLASH_ADDR, (uint16_t)(addr & 0xFFFF));
    flash_write_reg8(REG_FLASH_ADDR_HIGH, (uint8_t)((addr >> 16) & 0xFF));
}

/**
 * @brief 写使能
 */
static void flash_write_enable(void)
{
    flash_send_cmd(FLASH_CMD_WRITE_EN);
    flash_wait_ready();
}

/**
 * @brief 写禁止
 */
static void flash_write_disable(void)
{
    flash_send_cmd(FLASH_CMD_WRITE_DIS);
    flash_wait_ready();
}

/* ============================================================
 * 初始化
 * ============================================================ */
bool flash_init(void)
{
    if (flash_initialized) {
        return true;
    }
    
    // 读取Flash ID
    flash_send_cmd(FLASH_CMD_READ_ID);
    flash_wait_ready();
    
    // 根据ID判断容量
    // 这里简化处理，假设512KB
    flash_capacity = FLASH_TOTAL_SIZE;
    
    // 初始化日志索引
    log_write_index = 0;
    log_count = 0;
    
    // 扫描日志数量
    Flash_LogEntryTypeDef entry;
    for (uint16_t i = 0; i < (FLASH_SECTOR_SIZE / sizeof(Flash_LogEntryTypeDef)); i++) {
        if (flash_log_read(i, &entry)) {
            log_count++;
        } else {
            break;
        }
    }
    
    flash_initialized = true;
    
    return true;
}

/* ============================================================
 * 基本读写
 * ============================================================ */
bool flash_read(uint32_t addr, uint8_t *data, uint16_t len)
{
    if (!flash_initialized) return false;
    if (addr + len > flash_capacity) return false;
    if (data == NULL || len == 0) return false;
    
    irq_disable();
    
    // 发送读取命令
    flash_send_cmd(FLASH_CMD_READ);
    flash_set_addr(addr);
    
    // 读取数据
    for (uint16_t i = 0; i < len; i++) {
        data[i] = flash_read_reg8(REG_FLASH_DATA);
    }
    
    irq_enable();
    
    return true;
}

bool flash_write(uint32_t addr, uint8_t *data, uint16_t len)
{
    if (!flash_initialized) return false;
    if (addr + len > flash_capacity) return false;
    if (data == NULL || len == 0) return false;
    if (addr % FLASH_PAGE_SIZE != 0) return false;  // 必须页对齐
    
    irq_disable();
    
    uint16_t written = 0;
    while (written < len) {
        // 计算当前页剩余空间
        uint16_t page_remain = FLASH_PAGE_SIZE - ((addr + written) % FLASH_PAGE_SIZE);
        uint16_t to_write = (len - written) < page_remain ? (len - written) : page_remain;
        
        // 写使能
        flash_write_enable();
        
        // 发送页编程命令
        flash_send_cmd(FLASH_CMD_PAGE_PROG);
        flash_set_addr(addr + written);
        
        // 写入数据
        for (uint16_t i = 0; i < to_write; i++) {
            flash_write_reg8(REG_FLASH_DATA, data[written + i]);
        }
        
        flash_wait_ready();
        
        // 写禁止
        flash_write_disable();
        
        written += to_write;
    }
    
    irq_enable();
    
    return true;
}

bool flash_erase_sector(uint32_t addr)
{
    if (!flash_initialized) return false;
    if (addr % FLASH_SECTOR_SIZE != 0) return false;
    if (addr >= flash_capacity) return false;
    
    irq_disable();
    
    // 写使能
    flash_write_enable();
    
    // 发送扇区擦除命令
    flash_send_cmd(FLASH_CMD_SECTOR_ERASE);
    flash_set_addr(addr);
    
    flash_wait_ready();
    
    // 写禁止
    flash_write_disable();
    
    irq_enable();
    
    return true;
}

bool flash_erase_all(void)
{
    if (!flash_initialized) return false;
    
    irq_disable();
    
    // 写使能
    flash_write_enable();
    
    // 发送芯片擦除命令
    flash_send_cmd(FLASH_CMD_CHIP_ERASE);
    
    flash_wait_ready();
    
    // 写禁止
    flash_write_disable();
    
    irq_enable();
    
    return true;
}

/* ============================================================
 * 配置管理
 * ============================================================ */
bool flash_config_load(Flash_ConfigTypeDef *config)
{
    if (config == NULL) return false;
    
    // 从Flash读取配置
    if (!flash_read(FLASH_CONFIG_ADDR, (uint8_t *)config, sizeof(Flash_ConfigTypeDef))) {
        return false;
    }
    
    // 检查魔数
    if (config->magic != 0x54504859) {  // 'TPHY'
        return false;
    }
    
    // 验证CRC
    if (!flash_config_verify(config)) {
        return false;
    }
    
    return true;
}

bool flash_config_save(Flash_ConfigTypeDef *config)
{
    if (config == NULL) return false;
    
    // 设置魔数
    config->magic = 0x54504859;  // 'TPHY'
    config->version = 0x0100;     // v1.0
    
    // 计算CRC
    config->crc16 = flash_crc16((uint8_t *)config, 
                                 sizeof(Flash_ConfigTypeDef) - sizeof(uint16_t));
    
    // 擦除配置扇区
    if (!flash_erase_sector(FLASH_CONFIG_ADDR)) {
        return false;
    }
    
    // 写入配置
    if (!flash_write(FLASH_CONFIG_ADDR, (uint8_t *)config, sizeof(Flash_ConfigTypeDef))) {
        return false;
    }
    
    return true;
}

void flash_config_default(Flash_ConfigTypeDef *config)
{
    if (config == NULL) return;
    
    memset(config, 0, sizeof(Flash_ConfigTypeDef));
    
    config->magic = 0x54504859;
    config->version = 0x0100;
    
    // 默认治疗参数
    config->default_freq_hz = 10.0f;
    config->default_duty = 5000;        // 50%
    config->default_duration_s = 1200;  // 20分钟
    config->active_channels = 0x3F;     // 6通道全启用
    
    // 默认安全参数
    config->current_limit_ma = 20;
    config->impedance_min_ohm = 500;
    config->impedance_max_ohm = 10000;
    
    // 默认用户设置
    config->brightness = 128;           // 50%亮度
    config->volume = 50;                // 50%音量
    config->impedance_check = true;
    
    // 统计清零
    config->total_therapy_time_s = 0;
    config->total_sessions = 0;
    
    // 计算CRC
    config->crc16 = flash_crc16((uint8_t *)config, 
                                 sizeof(Flash_ConfigTypeDef) - sizeof(uint16_t));
}

bool flash_config_verify(Flash_ConfigTypeDef *config)
{
    if (config == NULL) return false;
    
    uint16_t crc = flash_crc16((uint8_t *)config, 
                                sizeof(Flash_ConfigTypeDef) - sizeof(uint16_t));
    
    return (crc == config->crc16);
}

/* ============================================================
 * 日志管理
 * ============================================================ */
bool flash_log_write(Flash_LogEntryTypeDef *entry)
{
    if (!flash_initialized || entry == NULL) return false;
    
    // 设置时间戳
    entry->timestamp = timer_get_tick_ms();
    
    // 计算CRC
    entry->crc16 = flash_crc16((uint8_t *)entry, 
                                sizeof(Flash_LogEntryTypeDef) - sizeof(uint16_t));
    
    // 计算写入地址
    uint32_t addr = FLASH_LOG_ADDR + (log_write_index * sizeof(Flash_LogEntryTypeDef));
    
    // 检查是否需要擦除扇区
    if (addr >= (FLASH_LOG_ADDR + FLASH_SECTOR_SIZE)) {
        // 擦除日志扇区
        if (!flash_erase_sector(FLASH_LOG_ADDR)) {
            return false;
        }
        log_write_index = 0;
        log_count = 0;
        addr = FLASH_LOG_ADDR;
    }
    
    // 写入日志条目
    // 注意：这里假设日志条目不超过页大小
    if (!flash_write(addr, (uint8_t *)entry, sizeof(Flash_LogEntryTypeDef))) {
        return false;
    }
    
    log_write_index++;
    if (log_count < (FLASH_SECTOR_SIZE / sizeof(Flash_LogEntryTypeDef))) {
        log_count++;
    }
    
    return true;
}

bool flash_log_read(uint16_t index, Flash_LogEntryTypeDef *entry)
{
    if (!flash_initialized || entry == NULL) return false;
    if (index >= log_count) return false;
    
    // 计算读取地址
    uint32_t addr = FLASH_LOG_ADDR + (index * sizeof(Flash_LogEntryTypeDef));
    
    // 读取日志条目
    if (!flash_read(addr, (uint8_t *)entry, sizeof(Flash_LogEntryTypeDef))) {
        return false;
    }
    
    // 验证CRC
    uint16_t crc = flash_crc16((uint8_t *)entry, 
                                sizeof(Flash_LogEntryTypeDef) - sizeof(uint16_t));
    if (crc != entry->crc16) {
        return false;
    }
    
    return true;
}

uint16_t flash_log_count(void)
{
    return log_count;
}

bool flash_log_clear(void)
{
    if (!flash_initialized) return false;
    
    if (!flash_erase_sector(FLASH_LOG_ADDR)) {
        return false;
    }
    
    log_write_index = 0;
    log_count = 0;
    
    return true;
}

/* ============================================================
 * 工具函数
 * ============================================================ */
uint16_t flash_crc16(uint8_t *data, uint16_t len)
{
    if (data == NULL || len == 0) return 0;
    
    uint16_t crc = 0xFFFF;
    
    for (uint16_t i = 0; i < len; i++) {
        crc ^= (uint16_t)data[i] << 8;
        
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 0x8000) {
                crc = (crc << 1) ^ 0x1021;  // CCITT多项式
            } else {
                crc <<= 1;
            }
        }
    }
    
    return crc;
}

bool flash_is_empty(uint32_t addr, uint16_t len)
{
    if (!flash_initialized) return false;
    if (addr + len > flash_capacity) return false;
    
    for (uint16_t i = 0; i < len; i++) {
        uint8_t data;
        if (!flash_read(addr + i, &data, 1)) {
            return false;
        }
        if (data != 0xFF) {
            return false;
        }
    }
    
    return true;
}
