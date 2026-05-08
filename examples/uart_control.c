/**
 * TLSR8269 六路脉冲发生器 - 串口控制示例
 * 
 * 通过串口命令动态控制六路脉冲输出
 * 波特率：115200
 * 
 * 命令格式：
 *   FREQ <ch> <freq_hz>      - 设置通道频率
 *   DUTY <ch> <duty_100x>    - 设置通道占空比 (0-10000)
 *   PHASE <ch> <phase_100x>  - 设置通道相位 (0-36000)
 *   ENABLE <ch>              - 使能通道
 *   DISABLE <ch>             - 禁止通道
 *   CONFIG <ch> <freq> <duty> <phase> - 配置通道
 *   DEMO1                    - 运行演示1 (不同频率)
 *   DEMO2                    - 运行演示2 (相同频率不同占空比)
 *   STOP                     - 停止所有通道
 *   START                    - 启动所有通道
 *   STATUS                   - 查询所有通道状态
 *   HELP                     - 显示帮助
 */

#include "tl_common.h"
#include "drivers.h"
#include "app_config.h"
#include "../main.c"

// ============================================================================
// 串口配置
// ============================================================================

#define UART_RX_BUFFER_SIZE     64
#define UART_TX_BUFFER_SIZE     256

static u8 s_rxBuffer[UART_RX_BUFFER_SIZE];
static u8 s_rxIndex = 0;
static u8 s_cmdReady = 0;

// ============================================================================
// 串口初始化
// ============================================================================

void uart_control_init(void)
{
    // 配置UART引脚
    gpio_set_func(GPIO_PB1, AS_UART_TX);
    gpio_set_func(GPIO_PB2, AS_UART_RX);
    
    // 初始化UART
    uart_init_baudrate(UART_BAUD_RATE, CLOCK_SYS_CLOCK_HZ, PARITY_NONE, STOP_BIT_ONE);
    uart_recbuff_init((unsigned char *)s_rxBuffer, UART_RX_BUFFER_SIZE);
    
    // 使能RX中断
    uart_set_rx_interrupt(1);
}

// ============================================================================
// 命令解析
// ============================================================================

void uart_send_string(const char *str)
{
    while (*str) {
        uart_send_byte(*str++);
    }
}

void uart_send_number(s32 num)
{
    char buf[16];
    char *p = buf + 15;
    *p = '\0';
    
    u8 isNegative = 0;
    if (num < 0) {
        isNegative = 1;
        num = -num;
    }
    
    do {
        *--p = '0' + (num % 10);
        num /= 10;
    } while (num > 0);
    
    if (isNegative) {
        *--p = '-';
    }
    
    uart_send_string(p);
}

void print_help(void)
{
    uart_send_string("\r\n=== TLSR8269 六路脉冲发生器 ===\r\n");
    uart_send_string("命令列表:\r\n");
    uart_send_string("  FREQ <ch(0-5)> <freq_hz>        - 设置频率\r\n");
    uart_send_string("  DUTY <ch(0-5)> <duty(0-10000)>  - 设置占空比(0.00%-100.00%)\r\n");
    uart_send_string("  PHASE <ch(0-5)> <phase(0-36000)>- 设置相位(0.00°-360.00°)\r\n");
    uart_send_string("  ENABLE <ch(0-5)>                - 使能通道\r\n");
    uart_send_string("  DISABLE <ch(0-5)>               - 禁止通道\r\n");
    uart_send_string("  CONFIG <ch> <freq> <duty> <phase>- 完整配置\r\n");
    uart_send_string("  DEMO1                           - 演示1:不同频率\r\n");
    uart_send_string("  DEMO2                           - 演示2:不同占空比\r\n");
    uart_send_string("  STOP                            - 停止所有\r\n");
    uart_send_string("  START                           - 启动所有\r\n");
    uart_send_string("  STATUS                          - 查询状态\r\n");
    uart_send_string("  HELP                            - 显示帮助\r\n");
    uart_send_string("================================\r\n");
}

void print_status(void)
{
    uart_send_string("\r\n通道状态:\r\n");
    uart_send_string("CH | 频率(Hz) | 占空比(%) | 相位(°) | 状态\r\n");
    uart_send_string("---|----------|-----------|---------|------\r\n");
    
    for (u8 i = 0; i < PULSE_CHANNEL_NUM; i++) {
        PulseChannelCfg_t *pch = &g_pulseChannels[i];
        
        uart_send_string(" ");
        uart_send_number(i);
        uart_send_string(" | ");
        uart_send_number(pch->frequency_hz);
        uart_send_string("    | ");
        uart_send_number(pch->duty_percent / 100);
        uart_send_string(".");
        uart_send_number(pch->duty_percent % 100);
        uart_send_string("     | ");
        uart_send_number(pch->phase_offset / 100);
        uart_send_string(".");
        uart_send_number(pch->phase_offset % 100);
        uart_send_string("    | ");
        uart_send_string(pch->enabled ? "ON " : "OFF");
        uart_send_string("\r\n");
    }
}

// 简单字符串比较
u8 str_equal(const char *a, const char *b)
{
    while (*a && *b) {
        if (*a != *b) return 0;
        a++;
        b++;
    }
    return (*a == *b);
}

// 解析整数
s32 parse_number(const char *str, u8 *consumed)
{
    s32 num = 0;
    u8 i = 0;
    u8 isNegative = 0;
    
    // 跳过空格
    while (str[i] == ' ' || str[i] == '\t') i++;
    
    // 处理负号
    if (str[i] == '-') {
        isNegative = 1;
        i++;
    }
    
    // 解析数字
    while (str[i] >= '0' && str[i] <= '9') {
        num = num * 10 + (str[i] - '0');
        i++;
    }
    
    if (consumed) *consumed = i;
    return isNegative ? -num : num;
}

void process_command(char *cmd)
{
    // 跳过前导空格
    while (*cmd == ' ' || *cmd == '\t') cmd++;
    
    // 提取命令关键字
    char keyword[16];
    u8 ki = 0;
    while (*cmd && *cmd != ' ' && *cmd != '\t' && ki < 15) {
        keyword[ki++] = *cmd++;
    }
    keyword[ki] = '\0';
    
    // 跳过参数前的空格
    while (*cmd == ' ' || *cmd == '\t') cmd++;
    
    // 解析命令
    if (str_equal(keyword, "HELP")) {
        print_help();
    }
    else if (str_equal(keyword, "STATUS")) {
        print_status();
    }
    else if (str_equal(keyword, "FREQ")) {
        u8 consumed;
        s32 ch = parse_number(cmd, &consumed);
        cmd += consumed;
        while (*cmd == ' ') cmd++;
        s32 freq = parse_number(cmd, &consumed);
        
        if (ch >= 0 && ch < PULSE_CHANNEL_NUM && freq > 0) {
            pulse_set_frequency((u8)ch, (u32)freq);
            uart_send_string("OK: CH");
            uart_send_number(ch);
            uart_send_string(" FREQ=");
            uart_send_number(freq);
            uart_send_string("Hz\r\n");
        } else {
            uart_send_string("ERROR: Invalid parameters\r\n");
        }
    }
    else if (str_equal(keyword, "DUTY")) {
        u8 consumed;
        s32 ch = parse_number(cmd, &consumed);
        cmd += consumed;
        while (*cmd == ' ') cmd++;
        s32 duty = parse_number(cmd, &consumed);
        
        if (ch >= 0 && ch < PULSE_CHANNEL_NUM && duty >= 0 && duty <= 10000) {
            pulse_set_duty((u8)ch, (u16)duty);
            uart_send_string("OK: CH");
            uart_send_number(ch);
            uart_send_string(" DUTY=");
            uart_send_number(duty / 100);
            uart_send_string(".");
            uart_send_number(duty % 100);
            uart_send_string("%\r\n");
        } else {
            uart_send_string("ERROR: Invalid parameters\r\n");
        }
    }
    else if (str_equal(keyword, "ENABLE")) {
        u8 consumed;
        s32 ch = parse_number(cmd, &consumed);
        
        if (ch >= 0 && ch < PULSE_CHANNEL_NUM) {
            pulse_channel_enable((u8)ch);
            uart_send_string("OK: CH");
            uart_send_number(ch);
            uart_send_string(" ENABLED\r\n");
        } else {
            uart_send_string("ERROR: Invalid channel\r\n");
        }
    }
    else if (str_equal(keyword, "DISABLE")) {
        u8 consumed;
        s32 ch = parse_number(cmd, &consumed);
        
        if (ch >= 0 && ch < PULSE_CHANNEL_NUM) {
            pulse_channel_disable((u8)ch);
            uart_send_string("OK: CH");
            uart_send_number(ch);
            uart_send_string(" DISABLED\r\n");
        } else {
            uart_send_string("ERROR: Invalid channel\r\n");
        }
    }
    else if (str_equal(keyword, "CONFIG")) {
        u8 consumed;
        s32 ch = parse_number(cmd, &consumed);
        cmd += consumed;
        while (*cmd == ' ') cmd++;
        s32 freq = parse_number(cmd, &consumed);
        cmd += consumed;
        while (*cmd == ' ') cmd++;
        s32 duty = parse_number(cmd, &consumed);
        cmd += consumed;
        while (*cmd == ' ') cmd++;
        s32 phase = parse_number(cmd, &consumed);
        
        if (ch >= 0 && ch < PULSE_CHANNEL_NUM && freq > 0 && 
            duty >= 0 && duty <= 10000 && phase >= 0 && phase <= 36000) {
            pulse_channel_config((u8)ch, (u32)freq, (u16)duty, (u16)phase);
            uart_send_string("OK: CH");
            uart_send_number(ch);
            uart_send_string(" CONFIGURED\r\n");
        } else {
            uart_send_string("ERROR: Invalid parameters\r\n");
        }
    }
    else if (str_equal(keyword, "DEMO1")) {
        pulse_demo_different_freq();
        uart_send_string("OK: Running demo1 (different frequencies)\r\n");
    }
    else if (str_equal(keyword, "DEMO2")) {
        pulse_demo_same_freq_diff_duty();
        uart_send_string("OK: Running demo2 (same freq, different duty)\r\n");
    }
    else if (str_equal(keyword, "STOP")) {
        for (u8 i = 0; i < PULSE_CHANNEL_NUM; i++) {
            pulse_channel_disable(i);
        }
        uart_send_string("OK: All channels stopped\r\n");
    }
    else if (str_equal(keyword, "START")) {
        for (u8 i = 0; i < PULSE_CHANNEL_NUM; i++) {
            pulse_channel_enable(i);
        }
        uart_send_string("OK: All channels started\r\n");
    }
    else {
        uart_send_string("ERROR: Unknown command. Type HELP for help.\r\n");
    }
}

// ============================================================================
// UART中断处理
// ============================================================================

_attribute_ram_code_ void uart_irq_handler(void)
{
    u8 irqSrc = reg_dma_rx_rdy0;
    
    if (irqSrc & FLD_DMA_UART_RX) {
        // 清除中断标志
        reg_dma_rx_rdy0 = FLD_DMA_UART_RX;
        
        // 读取接收到的数据
        u8 rxData = reg_uart_data_buf;
        
        // 处理回显
        uart_send_byte(rxData);
        
        // 处理接收字符
        if (rxData == '\r' || rxData == '\n') {
            if (s_rxIndex > 0) {
                s_rxBuffer[s_rxIndex] = '\0';
                s_cmdReady = 1;
                s_rxIndex = 0;
                uart_send_string("\r\n");
            }
        }
        else if (rxData == '\b' || rxData == 0x7F) {
            // 退格处理
            if (s_rxIndex > 0) {
                s_rxIndex--;
                uart_send_string(" \b");
            }
        }
        else if (s_rxIndex < UART_RX_BUFFER_SIZE - 1) {
            s_rxBuffer[s_rxIndex++] = rxData;
        }
    }
}

// ============================================================================
// 主函数扩展
// ============================================================================

void user_init_with_uart(void)
{
    // 初始化脉冲发生器
    pulse_generator_init();
    
    // 初始化串口控制
    uart_control_init();
    
    // 打印欢迎信息
    uart_send_string("\r\n");
    print_help();
    uart_send_string("> ");
}

void main_loop_with_uart(void)
{
    // 处理串口命令
    if (s_cmdReady) {
        s_cmdReady = 0;
        process_command((char *)s_rxBuffer);
        uart_send_string("> ");
    }
}
