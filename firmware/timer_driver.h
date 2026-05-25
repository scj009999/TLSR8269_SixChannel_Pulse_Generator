/**
 * @file timer_driver.h
 * @brief TLSR8269 定时器驱动 - 系统时钟和延时
 * @details 提供1ms系统Tick、微秒延时、看门狗功能
 * @author 束长江
 * @version 1.0.0
 * @date 2026-05-24
 */

#ifndef TIMER_DRIVER_H
#define TIMER_DRIVER_H

#include <stdint.h>
#include <stdbool.h>
#include "tlsr8269_reg.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================
 * 配置常量
 * ============================================================ */
#define TIMER_SYS_TICK_HZ       1000    // 系统Tick频率 1kHz = 1ms
#define TIMER_WATCHDOG_TIMEOUT_MS  1000  // 看门狗超时 1秒
#define TIMER_DELAY_MAX_US      1000000 // 最大延时1秒

/* ============================================================
 * 定时器编号
 * ============================================================ */
typedef enum {
    TIMER_0 = 0,    // Timer0 - 系统Tick
    TIMER_1 = 1,    // Timer1 - 通用定时
    TIMER_2 = 2,    // Timer2 - 备用
    TIMER_WD = 3    // Watchdog
} Timer_IDTypeDef;

/* ============================================================
 * 定时器模式
 * ============================================================ */
typedef enum {
    TIMER_MODE_FREE_RUN = 0,    // 自由运行
    TIMER_MODE_PERIODIC = 1,    // 周期模式
    TIMER_MODE_ONE_SHOT = 2     // 单次模式
} Timer_ModeTypeDef;

/* ============================================================
 * 回调函数类型
 * ============================================================ */
typedef void (*Timer_CallbackTypeDef)(void);

/* ============================================================
 * API函数声明
 * ============================================================ */

/**
 * @brief 初始化定时器系统
 * @return true成功，false失败
 */
bool timer_init(void);

/**
 * @brief 启动系统Tick定时器（1ms）
 * @param callback Tick回调函数
 * @return true成功，false失败
 */
bool timer_start_tick(Timer_CallbackTypeDef callback);

/**
 * @brief 停止系统Tick
 */
void timer_stop_tick(void);

/**
 * @brief 获取系统Tick计数（ms）
 * @return 毫秒计数
 */
uint32_t timer_get_tick_ms(void);

/**
 * @brief 获取系统运行时间（秒）
 * @return 秒数
 */
uint32_t timer_get_runtime_s(void);

/**
 * @brief 微秒延时（阻塞）
 * @param us 微秒数（最大1秒）
 */
void timer_delay_us(uint32_t us);

/**
 * @brief 毫秒延时（阻塞）
 * @param ms 毫秒数
 */
void timer_delay_ms(uint32_t ms);

/**
 * @brief 检查是否超时
 * @param start_ms 开始时间
 * @param timeout_ms 超时时间
 * @return true已超时，false未超时
 */
bool timer_is_timeout(uint32_t start_ms, uint32_t timeout_ms);

/**
 * @brief 初始化看门狗
 * @param timeout_ms 超时时间（ms）
 * @return true成功，false失败
 */
bool timer_watchdog_init(uint32_t timeout_ms);

/**
 * @brief 喂狗
 */
void timer_watchdog_feed(void);

/**
 * @brief 启动看门狗
 */
void timer_watchdog_start(void);

/**
 * @brief 停止看门狗
 */
void timer_watchdog_stop(void);

/**
 * @brief 通用定时器启动
 * @param id 定时器编号
 * @param period_us 周期（微秒）
 * @param callback 回调函数
 * @return true成功，false失败
 */
bool timer_start(Timer_IDTypeDef id, uint32_t period_us, Timer_CallbackTypeDef callback);

/**
 * @brief 停止通用定时器
 * @param id 定时器编号
 */
void timer_stop(Timer_IDTypeDef id);

/**
 * @brief 获取定时器计数
 * @param id 定时器编号
 * @return 当前计数值
 */
uint32_t timer_get_count(Timer_IDTypeDef id);

#ifdef __cplusplus
}
#endif

#endif /* TIMER_DRIVER_H */
