/**
 * @file uart_driver.c
 * @brief TLSR8269 UART驱动实现
 * @details 基于Telink SDK UART API，支持调试输出和命令处理
 */

#include "uart_driver.h"
#include "timer_driver.h"
#include <string.h>

/* ============================================================
 * 寄存器定义（基于Telink TLSR8269）
 * ============================================================ */

// UART0寄存器基地址
#define REG_UART0_BASE          0x90
#define REG_UART0_DATA          0x90   // 数据寄存器
#define REG_UART0_CTRL0         0x91   // 控制寄存器0
#define REG_UART0_CTRL1         0x92   // 控制寄存器1
#define REG_UART0_CTRL2         0x93   // 控制寄存器2
#define REG_UART0_STATUS        0x94   // 状态寄存器
#define REG_UART0_TIMEOUT       0x95   // 超时寄存器

// UART1寄存器基地址
#define REG_UART1_BASE          0x98
#define REG_UART1_DATA          0x98
#define REG_UART1_CTRL0         0x99
#define REG_UART1_CTRL1         0x9A
#define REG_UART1_CTRL2         0x9B
#define REG_UART1_STATUS        0x9C
#define REG_UART1_TIMEOUT       0x9D

// 控制位
#define UART_CTRL0_RX_EN        0x01   // 接收使能
#define UART_CTRL0_TX_EN        0x02   // 发送使能
#define UART_CTRL0_RX_IRQ_EN    0x04   // 接收中断使能
#define UART_CTRL0_TX_IRQ_EN    0x08   // 发送中断使能
#define UART_CTRL0_RX_DMA_EN    0x10   // 接收DMA使能
#define UART_CTRL0_TX_DMA_EN    0x20   // 发送DMA使能
#define UART_CTRL0_BWPC_MASK    0xC0   // 波特率分频掩码

// 状态位
#define UART_STATUS_RX_BUF_FULL 0x01   // 接收缓冲区满
#define UART_STATUS_TX_BUF_FULL 0x02   // 发送缓冲区满
#define UART_STATUS_RX_DONE     0x04   // 接收完成
#define UART_STATUS_TX_DONE     0x08   // 发送完成
#define UART_STATUS_RX_ERR      0x10   // 接收错误
#define UART_STATUS_CLEAR_ALL   0xFF   // 清除所有状态

/* ============================================================
 * 内部变量
 * ============================================================ */
typedef struct {
    uint8_t tx_buf[UART_TX_BUF_SIZE];
    volatile uint16_t tx_head;
    volatile uint16_t tx_tail;
    volatile bool tx_busy;
    
    uint8_t rx_buf[UART_RX_BUF_SIZE];
    volatile uint16_t rx_head;
    volatile uint16_t rx_tail;
    
    UART_RxCallbackTypeDef rx_callback;
    bool initialized;
} UART_ChannelTypeDef;

static UART_ChannelTypeDef uart_channels[2] = {0};
static Log_LevelTypeDef log_level = LOG_LEVEL_INFO;
static bool log_initialized = false;

/* ============================================================
 * 内联读写函数
 * ============================================================ */
static inline void uart_write8(uint16_t addr, uint8_t val) {
    *((volatile uint8_t *)addr) = val;
}

static inline uint8_t uart_read8(uint16_t addr) {
    return *((volatile uint8_t *)addr);
}

/* ============================================================
 * 获取寄存器地址
 * ============================================================ */
static uint16_t get_uart_data_reg(UART_IDTypeDef id) {
    return (id == UART_0) ? REG_UART0_DATA : REG_UART1_DATA;
}

static uint16_t get_uart_ctrl0_reg(UART_IDTypeDef id) {
    return (id == UART_0) ? REG_UART0_CTRL0 : REG_UART1_CTRL0;
}

static uint16_t get_uart_ctrl1_reg(UART_IDTypeDef id) {
    return (id == UART_0) ? REG_UART0_CTRL1 : REG_UART1_CTRL1;
}

static uint16_t get_uart_ctrl2_reg(UART_IDTypeDef id) {
    return (id == UART_0) ? REG_UART0_CTRL2 : REG_UART1_CTRL2;
}

static uint16_t get_uart_status_reg(UART_IDTypeDef id) {
    return (id == UART_0) ? REG_UART0_STATUS : REG_UART1_STATUS;
}

/* ============================================================
 * 计算波特率分频
 * ============================================================ */
static bool calculate_baudrate(uint32_t baudrate, uint8_t *bwpc, uint16_t *div)
{
    if (baudrate == 0 || baudrate > UART_MAX_BAUD) {
        return false;
    }
    
    // 系统时钟16MHz
    // baudrate = 16MHz / (div * (bwpc + 1))
    // 尝试不同的bwpc值，找到最佳分频
    
    uint32_t best_error = 0xFFFFFFFF;
    uint8_t best_bwpc = 0;
    uint16_t best_div = 0;
    
    for (uint8_t b = 3; b <= 15; b++) {
        uint32_t div_temp = 16000000 / (baudrate * (b + 1));
        
        if (div_temp < 1) div_temp = 1;
        if (div_temp > 0xFFFF) continue;
        
        uint32_t actual_baud = 16000000 / (div_temp * (b + 1));
        uint32_t error = (actual_baud > baudrate) ? 
                         (actual_baud - baudrate) : (baudrate - actual_baud);
        
        if (error < best_error) {
            best_error = error;
            best_bwpc = b;
            best_div = (uint16_t)div_temp;
        }
    }
    
    if (best_error > (baudrate / 20)) {  // 误差>5%
        return false;
    }
    
    *bwpc = best_bwpc;
    *div = best_div;
    
    return true;
}

/* ============================================================
 * 初始化
 * ============================================================ */
bool uart_init(UART_IDTypeDef id, UART_ConfigTypeDef *config)
{
    if (id > UART_1) return false;
    if (config == NULL) return false;
    
    UART_ChannelTypeDef *ch = &uart_channels[id];
    
    if (ch->initialized) {
        uart_deinit(id);
    }
    
    // 计算波特率
    uint8_t bwpc;
    uint16_t div;
    if (!calculate_baudrate(config->baudrate, &bwpc, &div)) {
        return false;
    }
    
    // 禁用UART
    uart_write8(get_uart_ctrl0_reg(id), 0);
    
    // 配置数据位、停止位、校验位
    uint8_t ctrl1 = 0;
    ctrl1 |= (config->databits & 0x03);
    ctrl1 |= ((config->stopbits & 0x03) << 2);
    ctrl1 |= ((config->parity & 0x03) << 4);
    uart_write8(get_uart_ctrl1_reg(id), ctrl1);
    
    // 配置波特率
    uint8_t ctrl2 = bwpc & 0x0F;
    uart_write8(get_uart_ctrl2_reg(id), ctrl2);
    
    // 设置分频（高8位在CTRL0，低8位在DATA）
    // 注意：实际实现可能不同，需参考Telink SDK
    
    // 清除状态
    uart_write8(get_uart_status_reg(id), UART_STATUS_CLEAR_ALL);
    
    // 使能接收和发送
    uint8_t ctrl0 = UART_CTRL0_RX_EN | UART_CTRL0_TX_EN;
    if (config->hw_flow_ctrl) {
        // 硬件流控使能
    }
    uart_write8(get_uart_ctrl0_reg(id), ctrl0);
    
    // 初始化缓冲区
    memset(ch, 0, sizeof(UART_ChannelTypeDef));
    ch->initialized = true;
    
    return true;
}

bool uart_init_default(UART_IDTypeDef id)
{
    UART_ConfigTypeDef config = {
        .baudrate = UART_DEFAULT_BAUD,
        .databits = UART_DATA_8BIT,
        .stopbits = UART_STOP_1BIT,
        .parity = UART_PARITY_NONE,
        .hw_flow_ctrl = false
    };
    
    return uart_init(id, &config);
}

void uart_deinit(UART_IDTypeDef id)
{
    if (id > UART_1) return;
    
    // 禁用UART
    uart_write8(get_uart_ctrl0_reg(id), 0);
    
    // 清除状态
    uart_write8(get_uart_status_reg(id), UART_STATUS_CLEAR_ALL);
    
    // 清除通道数据
    memset(&uart_channels[id], 0, sizeof(UART_ChannelTypeDef));
}

/* ============================================================
 * 发送
 * ============================================================ */
bool uart_send_byte(UART_IDTypeDef id, uint8_t data)
{
    if (id > UART_1) return false;
    
    UART_ChannelTypeDef *ch = &uart_channels[id];
    if (!ch->initialized) return false;
    
    // 检查发送缓冲区
    uint16_t next_tail = (ch->tx_tail + 1) % UART_TX_BUF_SIZE;
    if (next_tail == ch->tx_head) {
        // 缓冲区满
        return false;
    }
    
    // 放入缓冲区
    ch->tx_buf[ch->tx_tail] = data;
    ch->tx_tail = next_tail;
    
    // 如果不在发送中，启动发送
    if (!ch->tx_busy) {
        ch->tx_busy = true;
        
        // 发送第一个字节
        while (uart_read8(get_uart_status_reg(id)) & UART_STATUS_TX_BUF_FULL) {
            // 等待发送缓冲区空
        }
        
        if (ch->tx_head != ch->tx_tail) {
            uart_write8(get_uart_data_reg(id), ch->tx_buf[ch->tx_head]);
            ch->tx_head = (ch->tx_head + 1) % UART_TX_BUF_SIZE;
        } else {
            ch->tx_busy = false;
        }
    }
    
    return true;
}

bool uart_send(UART_IDTypeDef id, uint8_t *data, uint16_t len)
{
    if (id > UART_1 || data == NULL || len == 0) return false;
    
    for (uint16_t i = 0; i < len; i++) {
        if (!uart_send_byte(id, data[i])) {
            return false;
        }
    }
    
    return true;
}

bool uart_send_string(UART_IDTypeDef id, const char *str)
{
    if (str == NULL) return false;
    
    return uart_send(id, (uint8_t *)str, (uint16_t)strlen(str));
}

int uart_printf(UART_IDTypeDef id, const char *format, ...)
{
    if (id > UART_1) return -1;
    
    char buf[UART_TX_BUF_SIZE];
    va_list args;
    va_start(args, format);
    int len = vsnprintf(buf, sizeof(buf), format, args);
    va_end(args);
    
    if (len > 0) {
        uart_send(id, (uint8_t *)buf, (uint16_t)len);
    }
    
    return len;
}

/* ============================================================
 * 接收
 * ============================================================ */
bool uart_receive_byte(UART_IDTypeDef id, uint8_t *data)
{
    if (id > UART_1 || data == NULL) return false;
    
    UART_ChannelTypeDef *ch = &uart_channels[id];
    if (!ch->initialized) return false;
    
    // 检查接收缓冲区
    if (ch->rx_head == ch->rx_tail) {
        // 尝试从硬件读取
        uint8_t status = uart_read8(get_uart_status_reg(id));
        if (status & UART_STATUS_RX_DONE) {
            *data = uart_read8(get_uart_data_reg(id));
            uart_write8(get_uart_status_reg(id), UART_STATUS_RX_DONE);
            return true;
        }
        return false;
    }
    
    *data = ch->rx_buf[ch->rx_head];
    ch->rx_head = (ch->rx_head + 1) % UART_RX_BUF_SIZE;
    
    return true;
}

uint16_t uart_receive(UART_IDTypeDef id, uint8_t *data, uint16_t max_len)
{
    if (id > UART_1 || data == NULL || max_len == 0) return 0;
    
    uint16_t received = 0;
    while (received < max_len && uart_receive_byte(id, &data[received])) {
        received++;
    }
    
    return received;
}

void uart_set_rx_callback(UART_IDTypeDef id, UART_RxCallbackTypeDef callback)
{
    if (id > UART_1) return;
    
    uart_channels[id].rx_callback = callback;
}

void uart_flush_rx(UART_IDTypeDef id)
{
    if (id > UART_1) return;
    
    UART_ChannelTypeDef *ch = &uart_channels[id];
    ch->rx_head = 0;
    ch->rx_tail = 0;
    
    // 清除硬件接收状态
    uart_write8(get_uart_status_reg(id), UART_STATUS_RX_DONE);
}

void uart_flush_tx(UART_IDTypeDef id)
{
    if (id > UART_1) return;
    
    UART_ChannelTypeDef *ch = &uart_channels[id];
    ch->tx_head = 0;
    ch->tx_tail = 0;
    ch->tx_busy = false;
}

bool uart_is_tx_idle(UART_IDTypeDef id)
{
    if (id > UART_1) return true;
    
    UART_ChannelTypeDef *ch = &uart_channels[id];
    return (!ch->tx_busy && (ch->tx_head == ch->tx_tail));
}

uint16_t uart_rx_available(UART_IDTypeDef id)
{
    if (id > UART_1) return 0;
    
    UART_ChannelTypeDef *ch = &uart_channels[id];
    if (ch->rx_tail >= ch->rx_head) {
        return ch->rx_tail - ch->rx_head;
    } else {
        return UART_RX_BUF_SIZE - ch->rx_head + ch->rx_tail;
    }
}

/* ============================================================
 * 日志系统
 * ============================================================ */
bool log_init(Log_LevelTypeDef level)
{
    if (log_initialized) {
        return true;
    }
    
    // 初始化UART0用于日志
    if (!uart_init_default(UART_0)) {
        return false;
    }
    
    log_level = level;
    log_initialized = true;
    
    // 发送启动消息
    uart_send_string(UART_0, "\r\n===== Pulse Therapy Log =====\r\n");
    
    return true;
}

void log_set_level(Log_LevelTypeDef level)
{
    log_level = level;
}

Log_LevelTypeDef log_get_level(void)
{
    return log_level;
}

void log_write(Log_LevelTypeDef level, const char *tag, const char *format, ...)
{
    if (!log_initialized || level > log_level) {
        return;
    }
    
    const char *level_str;
    switch (level) {
        case LOG_LEVEL_ERROR:   level_str = "E"; break;
        case LOG_LEVEL_WARN:    level_str = "W"; break;
        case LOG_LEVEL_INFO:    level_str = "I"; break;
        case LOG_LEVEL_DEBUG:   level_str = "D"; break;
        case LOG_LEVEL_VERBOSE: level_str = "V"; break;
        default: return;
    }
    
    // 获取时间戳
    uint32_t tick = timer_get_tick_ms();
    uint32_t sec = tick / 1000;
    uint32_t ms = tick % 1000;
    
    // 格式化前缀
    char prefix[64];
    snprintf(prefix, sizeof(prefix), "[%02lu.%03lu][%s][%s] ", sec, ms, level_str, tag ? tag : "???");
    uart_send_string(UART_0, prefix);
    
    // 格式化消息
    char buf[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buf, sizeof(buf), format, args);
    va_end(args);
    
    uart_send_string(UART_0, buf);
    uart_send_string(UART_0, "\r\n");
}

/* ============================================================
 * 调试命令系统
 * ============================================================ */
#define DEBUG_CMD_BUF_SIZE  64
#define DEBUG_CMD_MAX_ARGS  8

static char debug_cmd_buf[DEBUG_CMD_BUF_SIZE];
static uint16_t debug_cmd_len = 0;
static bool debug_cmd_initialized = false;

typedef struct {
    const char *name;
    const char *help;
    void (*handler)(int argc, char *argv[]);
} Debug_CmdTypeDef;

// 命令处理函数声明
static void cmd_help(int argc, char *argv[]);
static void cmd_status(int argc, char *argv[]);
static void cmd_pwm(int argc, char *argv[]);
static void cmd_adc(int argc, char *argv[]);
static void cmd_therapy(int argc, char *argv[]);

static const Debug_CmdTypeDef debug_cmds[] = {
    {"help",    "Show help",                    cmd_help},
    {"status",  "Show system status",           cmd_status},
    {"pwm",     "PWM control: pwm <ch> <freq> <duty>", cmd_pwm},
    {"adc",     "ADC read: adc <ch>",           cmd_adc},
    {"therapy", "Therapy control: therapy <start|stop|pause>", cmd_therapy},
    {NULL, NULL, NULL}
};

bool debug_cmd_init(void)
{
    if (debug_cmd_initialized) {
        return true;
    }
    
    if (!log_initialized) {
        if (!log_init(LOG_LEVEL_INFO)) {
            return false;
        }
    }
    
    debug_cmd_len = 0;
    memset(debug_cmd_buf, 0, sizeof(debug_cmd_buf));
    debug_cmd_initialized = true;
    
    LOG_I("DEBUG", "Debug command system initialized");
    LOG_I("DEBUG", "Type 'help' for available commands");
    
    return true;
}

static void cmd_help(int argc, char *argv[])
{
    (void)argc;
    (void)argv;
    
    LOG_I("CMD", "Available commands:");
    for (int i = 0; debug_cmds[i].name != NULL; i++) {
        uart_printf(UART_0, "  %-10s - %s\r\n", debug_cmds[i].name, debug_cmds[i].help);
    }
}

static void cmd_status(int argc, char *argv[])
{
    (void)argc;
    (void)argv;
    
    LOG_I("CMD", "System Status:");
    LOG_I("CMD", "  Runtime: %lu s", timer_get_runtime_s());
    LOG_I("CMD", "  Tick: %lu ms", timer_get_tick_ms());
    // TODO: 添加更多状态信息
}

static void cmd_pwm(int argc, char *argv[])
{
    if (argc < 4) {
        LOG_E("CMD", "Usage: pwm <ch> <freq> <duty>");
        return;
    }
    
    uint8_t ch = (uint8_t)atoi(argv[1]);
    float freq = (float)atof(argv[2]);
    uint16_t duty = (uint16_t)atoi(argv[3]);
    
    if (ch > 5) {
        LOG_E("CMD", "Invalid channel: %d", ch);
        return;
    }
    
    // TODO: 调用PWM驱动
    LOG_I("CMD", "PWM CH%d: %.1fHz, %d/10000", ch, freq, duty);
}

static void cmd_adc(int argc, char *argv[])
{
    if (argc < 2) {
        LOG_E("CMD", "Usage: adc <ch>");
        return;
    }
    
    uint8_t ch = (uint8_t)atoi(argv[1]);
    if (ch > 5) {
        LOG_E("CMD", "Invalid channel: %d", ch);
        return;
    }
    
    // TODO: 调用ADC驱动
    LOG_I("CMD", "ADC CH%d: (TODO)", ch);
}

static void cmd_therapy(int argc, char *argv[])
{
    if (argc < 2) {
        LOG_E("CMD", "Usage: therapy <start|stop|pause>");
        return;
    }
    
    if (strcmp(argv[1], "start") == 0) {
        LOG_I("CMD", "Starting therapy...");
        // TODO: 调用治疗控制
    } else if (strcmp(argv[1], "stop") == 0) {
        LOG_I("CMD", "Stopping therapy...");
        // TODO: 调用治疗控制
    } else if (strcmp(argv[1], "pause") == 0) {
        LOG_I("CMD", "Pausing therapy...");
        // TODO: 调用治疗控制
    } else {
        LOG_E("CMD", "Unknown command: %s", argv[1]);
    }
}

void debug_cmd_process(void)
{
    if (!debug_cmd_initialized) return;
    
    // 读取接收到的字符
    uint8_t ch;
    while (uart_receive_byte(UART_0, &ch)) {
        // 回显
        uart_send_byte(UART_0, ch);
        
        if (ch == '\r' || ch == '\n') {
            // 命令结束
            uart_send_string(UART_0, "\r\n");
            
            if (debug_cmd_len > 0) {
                debug_cmd_buf[debug_cmd_len] = '\0';
                
                // 解析命令
                char *argv[DEBUG_CMD_MAX_ARGS];
                int argc = 0;
                char *token = strtok(debug_cmd_buf, " \t");
                
                while (token != NULL && argc < DEBUG_CMD_MAX_ARGS) {
                    argv[argc++] = token;
                    token = strtok(NULL, " \t");
                }
                
                if (argc > 0) {
                    // 查找并执行命令
                    bool found = false;
                    for (int i = 0; debug_cmds[i].name != NULL; i++) {
                        if (strcmp(argv[0], debug_cmds[i].name) == 0) {
                            debug_cmds[i].handler(argc, argv);
                            found = true;
                            break;
                        }
                    }
                    
                    if (!found) {
                        LOG_E("CMD", "Unknown command: %s", argv[0]);
                        LOG_I("CMD", "Type 'help' for available commands");
                    }
                }
                
                // 重置缓冲区
                debug_cmd_len = 0;
                memset(debug_cmd_buf, 0, sizeof(debug_cmd_buf));
            }
        } else if (ch == '\b' || ch == 0x7F) {
            // 退格
            if (debug_cmd_len > 0) {
                debug_cmd_len--;
                debug_cmd_buf[debug_cmd_len] = '\0';
                uart_send_string(UART_0, " \b");
            }
        } else if (debug_cmd_len < DEBUG_CMD_BUF_SIZE - 1) {
            // 添加字符
            debug_cmd_buf[debug_cmd_len++] = (char)ch;
        }
    }
}

/* ============================================================
 * 中断服务程序
 * ============================================================ */
void uart0_irq_handler(void)
{
    UART_ChannelTypeDef *ch = &uart_channels[UART_0];
    uint8_t status = uart_read8(REG_UART0_STATUS);
    
    // 接收中断
    if (status & UART_STATUS_RX_DONE) {
        uint8_t data = uart_read8(REG_UART0_DATA);
        
        // 放入接收缓冲区
        uint16_t next_tail = (ch->rx_tail + 1) % UART_RX_BUF_SIZE;
        if (next_tail != ch->rx_head) {
            ch->rx_buf[ch->rx_tail] = data;
            ch->rx_tail = next_tail;
        }
        
        // 调用回调
        if (ch->rx_callback != NULL) {
            ch->rx_callback(&data, 1);
        }
        
        // 清除状态
        uart_write8(REG_UART0_STATUS, UART_STATUS_RX_DONE);
    }
    
    // 发送中断
    if (status & UART_STATUS_TX_DONE) {
        if (ch->tx_head != ch->tx_tail) {
            uart_write8(REG_UART0_DATA, ch->tx_buf[ch->tx_head]);
            ch->tx_head = (ch->tx_head + 1) % UART_TX_BUF_SIZE;
        } else {
            ch->tx_busy = false;
        }
        
        // 清除状态
        uart_write8(REG_UART0_STATUS, UART_STATUS_TX_DONE);
    }
    
    // 清除其他状态
    uart_write8(REG_UART0_STATUS, status);
}

void uart1_irq_handler(void)
{
    // 类似uart0_irq_handler
    UART_ChannelTypeDef *ch = &uart_channels[UART_1];
    uint8_t status = uart_read8(REG_UART1_STATUS);
    
    if (status & UART_STATUS_RX_DONE) {
        uint8_t data = uart_read8(REG_UART1_DATA);
        uint16_t next_tail = (ch->rx_tail + 1) % UART_RX_BUF_SIZE;
        if (next_tail != ch->rx_head) {
            ch->rx_buf[ch->rx_tail] = data;
            ch->rx_tail = next_tail;
        }
        uart_write8(REG_UART1_STATUS, UART_STATUS_RX_DONE);
    }
    
    if (status & UART_STATUS_TX_DONE) {
        if (ch->tx_head != ch->tx_tail) {
            uart_write8(REG_UART1_DATA, ch->tx_buf[ch->tx_head]);
            ch->tx_head = (ch->tx_head + 1) % UART_TX_BUF_SIZE;
        } else {
            ch->tx_busy = false;
        }
        uart_write8(REG_UART1_STATUS, UART_STATUS_TX_DONE);
    }
    
    uart_write8(REG_UART1_STATUS, status);
}
