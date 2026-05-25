/**
 * @file timer_driver.c
 * @brief TLSR8269 定时器驱动实现
 * @details 基于Telink SDK Timer API
 */

#include "timer_driver.h"
#include "string.h"

/* ============================================================
 * 寄存器定义（基于Telink TLSR8269）
 * ============================================================ */

// 定时器寄存器基地址
#define REG_TIMER_BASE          0x620

// Timer0寄存器
#define REG_TIMER0_TICK         0x620  // Timer0当前计数值
#define REG_TIMER0_STATUS       0x621  // Timer0状态
#define REG_TIMER0_CTRL         0x622  // Timer0控制
#define REG_TIMER0_CAPT         0x623  // Timer0捕获值

// Timer1寄存器
#define REG_TIMER1_TICK         0x624
#define REG_TIMER1_STATUS       0x625
#define REG_TIMER1_CTRL         0x626
#define REG_TIMER1_CAPT         0x627

// Timer2寄存器
#define REG_TIMER2_TICK         0x628
#define REG_TIMER2_STATUS       0x629
#define REG_TIMER2_CTRL         0x62A
#define REG_TIMER2_CAPT         0x62B

// 系统时钟配置
#define REG_SYSTEM_TICK         0x740  // 系统Tick计数
#define REG_SYSTEM_CTRL         0x741  // 系统控制

// 看门狗寄存器
#define REG_WD_CTRL             0x642  // 看门狗控制
#define REG_WD_STATUS           0x643  // 看门狗状态

// 时钟源
#define TIMER_CLK_16M           0x00   // 16MHz
#define TIMER_CLK_4M            0x01   // 4MHz
#define TIMER_CLK_1M            0x02   // 1MHz

// 控制位
#define TIMER_CTRL_EN           0x01   // 使能
#define TIMER_CTRL_MODE_MASK    0x06   // 模式掩码
#define TIMER_CTRL_MODE_FREE    0x00   // 自由运行
#define TIMER_CTRL_MODE_PERIOD  0x02   // 周期模式
#define TIMER_CTRL_CLK_MASK     0x18   // 时钟掩码
#define TIMER_CTRL_IRQ_EN       0x20   // 中断使能

// 看门狗控制
#define WD_CTRL_EN              0x01   // 看门狗使能
#define WD_CTRL_CLR             0x02   // 清除计数
#define WD_CTRL_RST_EN          0x04   // 复位使能

/* ============================================================
 * 内部变量
 * ============================================================ */
static volatile uint32_t system_tick_ms = 0;      // 系统Tick计数
static volatile uint32_t runtime_seconds = 0;     // 运行时间（秒）
static Timer_CallbackTypeDef tick_callback = NULL; // Tick回调
static Timer_CallbackTypeDef timer_callbacks[3] = {NULL, NULL, NULL}; // 定时器回调
static bool timer_initialized = false;
static bool watchdog_enabled = false;

/* ============================================================
 * 内联读写函数
 * ============================================================ */
static inline void reg_write8(uint16_t addr, uint8_t val) {
    *((volatile uint8_t *)addr) = val;
}

static inline uint8_t reg_read8(uint16_t addr) {
    return *((volatile uint8_t *)addr);
}

static inline void reg_write16(uint16_t addr, uint16_t val) {
    *((volatile uint16_t *)addr) = val;
}

static inline uint16_t reg_read16(uint16_t addr) {
    return *((volatile uint16_t *)addr);
}

static inline void reg_write32(uint16_t addr, uint32_t val) {
    *((volatile uint32_t *)addr) = val;
}

static inline uint32_t reg_read32(uint16_t addr) {
    return *((volatile uint32_t *)addr);
}

/* ============================================================
 * 中断服务程序声明
 * ============================================================ */
void timer0_irq_handler(void);
void timer1_irq_handler(void);
void timer2_irq_handler(void);

/* ============================================================
 * 初始化
 * ============================================================ */
bool timer_init(void)
{
    if (timer_initialized) {
        return true;
    }
    
    // 禁用所有定时器中断
    irq_disable();
    
    // 清零定时器控制寄存器
    reg_write8(REG_TIMER0_CTRL, 0);
    reg_write8(REG_TIMER1_CTRL, 0);
    reg_write8(REG_TIMER2_CTRL, 0);
    
    // 清零看门狗
    reg_write8(REG_WD_CTRL, WD_CTRL_CLR);
    
    // 清零系统Tick
    system_tick_ms = 0;
    runtime_seconds = 0;
    
    // 清除回调
    tick_callback = NULL;
    memset((void *)timer_callbacks, 0, sizeof(timer_callbacks));
    
    timer_initialized = true;
    watchdog_enabled = false;
    
    irq_enable();
    
    return true;
}

/* ============================================================
 * 系统Tick（1ms）
 * ============================================================ */
bool timer_start_tick(Timer_CallbackTypeDef callback)
{
    if (!timer_initialized) {
        if (!timer_init()) {
            return false;
        }
    }
    
    irq_disable();
    
    // 保存回调
    tick_callback = callback;
    
    // 配置Timer0为周期模式，1ms中断
    // 时钟源：16MHz，分频16 = 1MHz，计数1000 = 1ms
    uint16_t period = 1000;  // 1ms @ 1MHz
    
    // 停止定时器
    reg_write8(REG_TIMER0_CTRL, 0);
    
    // 设置周期
    reg_write16(REG_TIMER0_CAPT, period);
    
    // 配置控制寄存器
    // 使能 + 周期模式 + 1MHz时钟 + 中断使能
    uint8_t ctrl = TIMER_CTRL_EN | TIMER_CTRL_MODE_PERIOD | 
                   (TIMER_CLK_1M << 3) | TIMER_CTRL_IRQ_EN;
    reg_write8(REG_TIMER0_CTRL, ctrl);
    
    // 清零计数
    reg_write16(REG_TIMER0_TICK, 0);
    
    // 清除中断标志
    reg_write8(REG_TIMER0_STATUS, 0xFF);
    
    irq_enable();
    
    return true;
}

void timer_stop_tick(void)
{
    irq_disable();
    
    // 禁用Timer0
    reg_write8(REG_TIMER0_CTRL, 0);
    tick_callback = NULL;
    
    irq_enable();
}

uint32_t timer_get_tick_ms(void)
{
    return system_tick_ms;
}

uint32_t timer_get_runtime_s(void)
{
    return runtime_seconds;
}

/* ============================================================
 * 延时函数
 * ============================================================ */
void timer_delay_us(uint32_t us)
{
    if (us == 0) return;
    if (us > TIMER_DELAY_MAX_US) us = TIMER_DELAY_MAX_US;
    
    // 使用Timer2进行精确延时
    irq_disable();
    
    // 配置Timer2为单次模式
    uint16_t count = (uint16_t)(us);  // 1MHz时钟，1计数=1us
    
    reg_write8(REG_TIMER2_CTRL, 0);
    reg_write16(REG_TIMER2_CAPT, count);
    reg_write8(REG_TIMER2_CTRL, TIMER_CTRL_EN | TIMER_CTRL_MODE_ONE_SHOT | 
               (TIMER_CLK_1M << 3));
    reg_write16(REG_TIMER2_TICK, 0);
    
    irq_enable();
    
    // 等待完成
    while (!(reg_read8(REG_TIMER2_STATUS) & 0x01)) {
        // 忙等待
    }
    
    // 清除标志
    reg_write8(REG_TIMER2_STATUS, 0xFF);
    reg_write8(REG_TIMER2_CTRL, 0);
}

void timer_delay_ms(uint32_t ms)
{
    while (ms > 0) {
        timer_delay_us(1000);
        ms--;
        
        // 喂狗（如果使能）
        if (watchdog_enabled) {
            timer_watchdog_feed();
        }
    }
}

/* ============================================================
 * 超时检查
 * ============================================================ */
bool timer_is_timeout(uint32_t start_ms, uint32_t timeout_ms)
{
    uint32_t elapsed = timer_get_tick_ms() - start_ms;
    return (elapsed >= timeout_ms);
}

/* ============================================================
 * 看门狗
 * ============================================================ */
bool timer_watchdog_init(uint32_t timeout_ms)
{
    if (!timer_initialized) {
        if (!timer_init()) {
            return false;
        }
    }
    
    irq_disable();
    
    // 配置看门狗超时
    // 看门狗时钟约为32kHz，每个计数约31us
    uint16_t count = (uint16_t)((timeout_ms * 1000) / 31);
    if (count < 100) count = 100;  // 最小约3ms
    if (count > 0xFFFF) count = 0xFFFF;  // 最大约2秒
    
    // 配置看门狗
    reg_write8(REG_WD_CTRL, WD_CTRL_CLR);  // 先清除
    reg_write16(REG_WD_STATUS, count);      // 设置超时值
    
    watchdog_enabled = false;
    
    irq_enable();
    
    return true;
}

void timer_watchdog_feed(void)
{
    if (!watchdog_enabled) return;
    
    // 清除看门狗计数
    reg_write8(REG_WD_CTRL, WD_CTRL_CLR);
}

void timer_watchdog_start(void)
{
    if (!timer_initialized) return;
    
    irq_disable();
    
    // 使能看门狗，允许复位
    reg_write8(REG_WD_CTRL, WD_CTRL_EN | WD_CTRL_RST_EN);
    watchdog_enabled = true;
    
    irq_enable();
}

void timer_watchdog_stop(void)
{
    irq_disable();
    
    // 禁用看门狗
    reg_write8(REG_WD_CTRL, WD_CTRL_CLR);
    watchdog_enabled = false;
    
    irq_enable();
}

/* ============================================================
 * 通用定时器
 * ============================================================ */
bool timer_start(Timer_IDTypeDef id, uint32_t period_us, Timer_CallbackTypeDef callback)
{
    if (!timer_initialized) {
        if (!timer_init()) {
            return false;
        }
    }
    
    if (id > TIMER_2) return false;
    if (period_us < 10 || period_us > 1000000) return false;
    
    uint16_t reg_base;
    switch (id) {
        case TIMER_1: reg_base = REG_TIMER1_TICK; break;
        case TIMER_2: reg_base = REG_TIMER2_TICK; break;
        default: return false;
    }
    
    irq_disable();
    
    // 保存回调
    timer_callbacks[id] = callback;
    
    // 计算周期
    uint16_t period = (uint16_t)(period_us);  // 1MHz时钟
    
    // 停止定时器
    reg_write8(reg_base + 2, 0);  // CTRL
    
    // 设置周期
    reg_write16(reg_base + 3, period);  // CAPT
    
    // 配置控制寄存器
    uint8_t ctrl = TIMER_CTRL_EN | TIMER_CTRL_MODE_PERIOD | 
                   (TIMER_CLK_1M << 3) | TIMER_CTRL_IRQ_EN;
    reg_write8(reg_base + 2, ctrl);
    
    // 清零计数
    reg_write16(reg_base, 0);
    
    // 清除中断标志
    reg_write8(reg_base + 1, 0xFF);
    
    irq_enable();
    
    return true;
}

void timer_stop(Timer_IDTypeDef id)
{
    if (id > TIMER_2) return;
    
    uint16_t reg_base;
    switch (id) {
        case TIMER_1: reg_base = REG_TIMER1_TICK; break;
        case TIMER_2: reg_base = REG_TIMER2_TICK; break;
        default: return;
    }
    
    irq_disable();
    
    reg_write8(reg_base + 2, 0);  // 禁用
    timer_callbacks[id] = NULL;
    
    irq_enable();
}

uint32_t timer_get_count(Timer_IDTypeDef id)
{
    if (id > TIMER_2) return 0;
    
    uint16_t reg_base;
    switch (id) {
        case TIMER_0: reg_base = REG_TIMER0_TICK; break;
        case TIMER_1: reg_base = REG_TIMER1_TICK; break;
        case TIMER_2: reg_base = REG_TIMER2_TICK; break;
        default: return 0;
    }
    
    return reg_read16(reg_base);
}

/* ============================================================
 * 中断服务程序
 * ============================================================ */
void timer0_irq_handler(void)
{
    // 清除中断标志
    reg_write8(REG_TIMER0_STATUS, 0xFF);
    
    // 更新系统Tick
    system_tick_ms++;
    
    // 更新运行时间
    if (system_tick_ms % 1000 == 0) {
        runtime_seconds++;
    }
    
    // 调用用户回调
    if (tick_callback != NULL) {
        tick_callback();
    }
    
    // 喂狗
    if (watchdog_enabled) {
        timer_watchdog_feed();
    }
}

void timer1_irq_handler(void)
{
    // 清除中断标志
    reg_write8(REG_TIMER1_STATUS, 0xFF);
    
    // 调用回调
    if (timer_callbacks[TIMER_1] != NULL) {
        timer_callbacks[TIMER_1]();
    }
}

void timer2_irq_handler(void)
{
    // 清除中断标志
    reg_write8(REG_TIMER2_STATUS, 0xFF);
    
    // 调用回调
    if (timer_callbacks[TIMER_2] != NULL) {
        timer_callbacks[TIMER_2]();
    }
}

/* ============================================================
 * 兼容旧版delay函数
 * ============================================================ */
// 这些函数在main_master_v3.c中使用
void delay_ms(uint32_t ms) {
    timer_delay_ms(ms);
}

void delay_us(uint32_t us) {
    timer_delay_us(us);
}

void irq_disable(void) {
    // 禁用全局中断
    // 具体实现取决于Telink SDK
    // asm("di");
}

void irq_enable(void) {
    // 使能全局中断
    // 具体实现取决于Telink SDK
    // asm("ei");
}
