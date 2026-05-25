/**
 * @file uart_driver.h
 * @brief TLSR8269 UART驱动 - 调试输出和日志
 * @details 提供串口初始化、发送、接收、printf重定向
 * @author 束长江
 * @version 1.0.0
 * @date 2026-05-24
 */

#ifndef UART_DRIVER_H
#define UART_DRIVER_H

#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include "tlsr8269_reg.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================
 * 配置常量
 * ============================================================ */
#define UART_TX_BUF_SIZE    256     // 发送缓冲区大小
#define UART_RX_BUF_SIZE    128     // 接收缓冲区大小
#define UART_DEFAULT_BAUD   115200  // 默认波特率
#define UART_MAX_BAUD       1000000 // 最大波特率1Mbps

/* ============================================================
 * UART编号
 * ============================================================ */
typedef enum {
    UART_0 = 0,     // UART0 - 默认调试口
    UART_1 = 1      // UART1 - 备用
} UART_IDTypeDef;

/* ============================================================
 * 数据位
 * ============================================================ */
typedef enum {
    UART_DATA_5BIT = 0,
    UART_DATA_6BIT = 1,
    UART_DATA_7BIT = 2,
    UART_DATA_8BIT = 3
} UART_DataBitsTypeDef;

/* ============================================================
 * 停止位
 * ============================================================ */
typedef enum {
    UART_STOP_1BIT = 0,
    UART_STOP_1_5BIT = 1,
    UART_STOP_2BIT = 2
} UART_StopBitsTypeDef;

/* ============================================================
 * 校验位
 * ============================================================ */
typedef enum {
    UART_PARITY_NONE = 0,
    UART_PARITY_EVEN = 1,
    UART_PARITY_ODD = 2
} UART_ParityTypeDef;

/* ============================================================
 * 配置结构体
 * ============================================================ */
typedef struct {
    uint32_t baudrate;              // 波特率
    UART_DataBitsTypeDef databits;  // 数据位
    UART_StopBitsTypeDef stopbits;  // 停止位
    UART_ParityTypeDef parity;      // 校验位
    bool hw_flow_ctrl;              // 硬件流控
} UART_ConfigTypeDef;

/* ============================================================
 * 日志级别
 * ============================================================ */
typedef enum {
    LOG_LEVEL_NONE = 0,     // 无日志
    LOG_LEVEL_ERROR = 1,    // 仅错误
    LOG_LEVEL_WARN = 2,     // 警告+错误
    LOG_LEVEL_INFO = 3,     // 信息+警告+错误
    LOG_LEVEL_DEBUG = 4,    // 调试+信息+警告+错误
    LOG_LEVEL_VERBOSE = 5   // 全部
} Log_LevelTypeDef;

/* ============================================================
 * 回调函数类型
 * ============================================================ */
typedef void (*UART_RxCallbackTypeDef)(uint8_t *data, uint16_t len);

/* ============================================================
 * API函数声明
 * ============================================================ */

/**
 * @brief 初始化UART
 * @param id UART编号
 * @param config 配置参数
 * @return true成功，false失败
 */
bool uart_init(UART_IDTypeDef id, UART_ConfigTypeDef *config);

/**
 * @brief 使用默认参数初始化（115200, 8N1）
 * @param id UART编号
 * @return true成功，false失败
 */
bool uart_init_default(UART_IDTypeDef id);

/**
 * @brief 反初始化UART
 * @param id UART编号
 */
void uart_deinit(UART_IDTypeDef id);

/**
 * @brief 发送一个字节
 * @param id UART编号
 * @param data 数据
 * @return true成功，false失败
 */
bool uart_send_byte(UART_IDTypeDef id, uint8_t data);

/**
 * @brief 发送数据
 * @param id UART编号
 * @param data 数据指针
 * @param len 长度
 * @return true成功，false失败
 */
bool uart_send(UART_IDTypeDef id, uint8_t *data, uint16_t len);

/**
 * @brief 发送字符串
 * @param id UART编号
 * @param str 字符串
 * @return true成功，false失败
 */
bool uart_send_string(UART_IDTypeDef id, const char *str);

/**
 * @brief 格式化发送（类似printf）
 * @param id UART编号
 * @param format 格式字符串
 * @param ... 可变参数
 * @return 发送的字节数
 */
int uart_printf(UART_IDTypeDef id, const char *format, ...);

/**
 * @brief 接收一个字节（非阻塞）
 * @param id UART编号
 * @param data 接收缓冲区
 * @return true有数据，false无数据
 */
bool uart_receive_byte(UART_IDTypeDef id, uint8_t *data);

/**
 * @brief 接收数据（非阻塞）
 * @param id UART编号
 * @param data 接收缓冲区
 * @param max_len 最大长度
 * @return 实际接收长度
 */
uint16_t uart_receive(UART_IDTypeDef id, uint8_t *data, uint16_t max_len);

/**
 * @brief 设置接收回调
 * @param id UART编号
 * @param callback 回调函数
 */
void uart_set_rx_callback(UART_IDTypeDef id, UART_RxCallbackTypeDef callback);

/**
 * @brief 清空接收缓冲区
 * @param id UART编号
 */
void uart_flush_rx(UART_IDTypeDef id);

/**
 * @brief 清空发送缓冲区
 * @param id UART编号
 */
void uart_flush_tx(UART_IDTypeDef id);

/**
 * @brief 检查发送完成
 * @param id UART编号
 * @return true空闲，false发送中
 */
bool uart_is_tx_idle(UART_IDTypeDef id);

/**
 * @brief 检查接收数据可用
 * @param id UART编号
 * @return 可用字节数
 */
uint16_t uart_rx_available(UART_IDTypeDef id);

/* ============================================================
 * 日志系统
 * ============================================================ */

/**
 * @brief 初始化日志系统
 * @param level 日志级别
 * @return true成功，false失败
 */
bool log_init(Log_LevelTypeDef level);

/**
 * @brief 设置日志级别
 * @param level 日志级别
 */
void log_set_level(Log_LevelTypeDef level);

/**
 * @brief 获取日志级别
 * @return 当前日志级别
 */
Log_LevelTypeDef log_get_level(void);

/**
 * @brief 输出日志
 * @param level 日志级别
 * @param tag 标签
 * @param format 格式字符串
 * @param ... 可变参数
 */
void log_write(Log_LevelTypeDef level, const char *tag, const char *format, ...);

/* 便捷宏 */
#define LOG_E(tag, ...) log_write(LOG_LEVEL_ERROR, tag, __VA_ARGS__)
#define LOG_W(tag, ...) log_write(LOG_LEVEL_WARN, tag, __VA_ARGS__)
#define LOG_I(tag, ...) log_write(LOG_LEVEL_INFO, tag, __VA_ARGS__)
#define LOG_D(tag, ...) log_write(LOG_LEVEL_DEBUG, tag, __VA_ARGS__)
#define LOG_V(tag, ...) log_write(LOG_LEVEL_VERBOSE, tag, __VA_ARGS__)

/* ============================================================
 * 调试命令系统
 * ============================================================ */

/**
 * @brief 初始化调试命令系统
 * @return true成功，false失败
 */
bool debug_cmd_init(void);

/**
 * @brief 处理调试命令（在主循环中调用）
 */
void debug_cmd_process(void);

#ifdef __cplusplus
}
#endif

#endif /* UART_DRIVER_H */
