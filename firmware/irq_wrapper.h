/**
 * @file irq_wrapper.h
 * @brief 中断控制包装器（兼容层）
 * @details 提供irq_disable/irq_enable函数
 */

#ifndef __IRQ_WRAPPER_H__
#define __IRQ_WRAPPER_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 禁用全局中断
 */
static inline void irq_disable(void)
{
    // TC32架构禁用中断
    __asm__ volatile("di" ::: "memory");
}

/**
 * @brief 启用全局中断
 */
static inline void irq_enable(void)
{
    // TC32架构启用中断
    __asm__ volatile("ei" ::: "memory");
}

#ifdef __cplusplus
}
#endif

#endif /* __IRQ_WRAPPER_H__ */
