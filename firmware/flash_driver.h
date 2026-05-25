/**
 * @file flash_driver.h
 * @brief TLSR8269 Flash存储驱动 - 参数持久化
 * @details 提供Flash读写、参数存储、配置保存功能
 * @author 束长江
 * @version 1.0.0
 * @date 2026-05-24
 */

#ifndef FLASH_DRIVER_H
#define FLASH_DRIVER_H

#include <stdint.h>
#include <stdbool.h>
#include "tlsr8269_reg.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================
 * 配置常量
 * ============================================================ */
#define FLASH_SECTOR_SIZE       4096    // Flash扇区大小 4KB
#define FLASH_PAGE_SIZE         256     // Flash页大小 256B
#define FLASH_TOTAL_SIZE        512000  // 总容量 512KB (CR8269F512)
#define FLASH_BASE_ADDR         0x00000 // Flash基地址
#define FLASH_CONFIG_ADDR       0x7E000 // 配置存储地址（倒数第2扇区）
#define FLASH_LOG_ADDR          0x7F000 // 日志存储地址（最后扇区）

/* ============================================================
 * 配置参数结构体
 * ============================================================ */
typedef struct {
    uint32_t magic;             // 魔数 0x54504859 ('TPHY')
    uint16_t version;           // 配置版本
    
    // 治疗参数
    float default_freq_hz;      // 默认频率
    uint16_t default_duty;      // 默认占空比
    uint16_t default_duration_s; // 默认时长
    uint8_t active_channels;    // 活动通道
    
    // 安全参数
    uint16_t current_limit_ma;  // 电流限制
    uint16_t impedance_min_ohm; // 最小阻抗
    uint16_t impedance_max_ohm; // 最大阻抗
    
    // 用户设置
    uint8_t brightness;         // 屏幕亮度
    uint8_t volume;             // 音量
    bool impedance_check;       // 阻抗检测开关
    
    // 统计
    uint32_t total_therapy_time_s;  // 总治疗时间
    uint32_t total_sessions;        // 总治疗次数
    
    // 校验
    uint16_t crc16;             // CRC校验
} Flash_ConfigTypeDef;

/* ============================================================
 * 日志条目结构体
 * ============================================================ */
typedef struct {
    uint32_t timestamp;         // 时间戳
    uint8_t event_type;         // 事件类型
    uint8_t channel;            // 通道
    uint16_t value;             // 值
    uint16_t crc16;             // CRC校验
} Flash_LogEntryTypeDef;

/* 事件类型 */
#define LOG_EVENT_START         0x01    // 治疗开始
#define LOG_EVENT_STOP          0x02    // 治疗停止
#define LOG_EVENT_PAUSE         0x03    // 治疗暂停
#define LOG_EVENT_RESUME        0x04    // 治疗恢复
#define LOG_EVENT_FAULT         0x05    // 故障
#define LOG_EVENT_EMERGENCY     0x06    // 急停
#define LOG_EVENT_CONFIG_CHANGE 0x07    // 配置变更

/* ============================================================
 * API函数声明
 * ============================================================ */

/**
 * @brief 初始化Flash驱动
 * @return true成功，false失败
 */
bool flash_init(void);

/**
 * @brief 读取Flash
 * @param addr 地址
 * @param data 数据缓冲区
 * @param len 长度
 * @return true成功，false失败
 */
bool flash_read(uint32_t addr, uint8_t *data, uint16_t len);

/**
 * @brief 写入Flash（自动擦除）
 * @param addr 地址（必须按页对齐）
 * @param data 数据
 * @param len 长度
 * @return true成功，false失败
 */
bool flash_write(uint32_t addr, uint8_t *data, uint16_t len);

/**
 * @brief 擦除扇区
 * @param addr 扇区地址
 * @return true成功，false失败
 */
bool flash_erase_sector(uint32_t addr);

/**
 * @brief 擦除整个Flash（危险！）
 * @return true成功，false失败
 */
bool flash_erase_all(void);

/* ============================================================
 * 配置管理
 * ============================================================ */

/**
 * @brief 加载配置
 * @param config 配置结构体
 * @return true成功，false使用默认值
 */
bool flash_config_load(Flash_ConfigTypeDef *config);

/**
 * @brief 保存配置
 * @param config 配置结构体
 * @return true成功，false失败
 */
bool flash_config_save(Flash_ConfigTypeDef *config);

/**
 * @brief 恢复默认配置
 * @param config 配置结构体
 */
void flash_config_default(Flash_ConfigTypeDef *config);

/**
 * @brief 验证配置CRC
 * @param config 配置结构体
 * @return true有效，false无效
 */
bool flash_config_verify(Flash_ConfigTypeDef *config);

/* ============================================================
 * 日志管理
 * ============================================================ */

/**
 * @brief 写入日志
 * @param entry 日志条目
 * @return true成功，false失败
 */
bool flash_log_write(Flash_LogEntryTypeDef *entry);

/**
 * @brief 读取日志
 * @param index 日志索引（0=最新）
 * @param entry 日志条目
 * @return true成功，false失败
 */
bool flash_log_read(uint16_t index, Flash_LogEntryTypeDef *entry);

/**
 * @brief 获取日志数量
 * @return 日志条目数
 */
uint16_t flash_log_count(void);

/**
 * @brief 清空日志
 * @return true成功，false失败
 */
bool flash_log_clear(void);

/* ============================================================
 * 工具函数
 * ============================================================ */

/**
 * @brief 计算CRC16
 * @param data 数据
 * @param len 长度
 * @return CRC16值
 */
uint16_t flash_crc16(uint8_t *data, uint16_t len);

/**
 * @brief 检查Flash是否为空
 * @param addr 地址
 * @param len 长度
 * @return true为空，false非空
 */
bool flash_is_empty(uint32_t addr, uint16_t len);

#ifdef __cplusplus
}
#endif

#endif /* FLASH_DRIVER_H */
