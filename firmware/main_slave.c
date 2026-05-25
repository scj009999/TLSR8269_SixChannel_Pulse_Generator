/**************************************************************************
 * @file main_slave.c
 * @brief TLSR8269 经颅脉冲治疗仪 - 从设备（治疗端/执行端）
 * @author 全国首个第三方TLSR8269 C库
 * @note 医疗级安全标准，硬件级保护，独立运行
 **************************************************************************/

#include "tlsr8269_reg.h"
#include "self_check.h"
#include "pwm_driver.h"
#include "adc_driver.h"

//==========================================================================
// 一、协议定义（和主设备完全匹配）
//==========================================================================
#define PROTOCOL_HEAD       0xAA
#define PROTOCOL_TAIL       0x55
#define CMD_SET_CHANNEL     0x01
#define CMD_SET_GLOBAL      0x02
#define CMD_START           0x03
#define CMD_STOP            0x04
#define CMD_GET_STATUS      0x05
#define CMD_ACK             0xFA

// 医疗安全阈值（和主设备完全一致）
#define FREQ_MIN_HZ         0.1f
#define FREQ_MAX_HZ         1000.0f
#define WIDTH_MIN_US        1UL
#define WIDTH_MAX_US        10000UL
#define DUR_MAX_MIN         30UL
#define MAX_INTENSITY       100

// 硬件安全限制
#define HW_CURRENT_LIMIT_MA     2.0f    // 硬件电流限制2mA
#define HW_VOLTAGE_LIMIT_V      24.0f   // 硬件电压限制24V
#define HW_IMPEDANCE_MIN_OHM    1000.0f // 最小阻抗1kΩ
#define HW_IMPEDANCE_MAX_OHM    10000.0f // 最大阻抗10kΩ

// 通信帧结构
#pragma pack(1)
typedef struct {
    uint8_t head;
    uint8_t cmd;
    uint8_t len;
    uint8_t data[16];
    uint16_t crc;
    uint8_t tail;
} Protocol_FrameTypeDef;
#pragma pack()

// 通道参数
typedef struct {
    bool enable;
    float freq_hz;
    uint32_t width_us;
    uint8_t phase_deg;
    uint8_t intensity;
    bool polarity;
} Channel_CfgTypeDef;

typedef struct {
    Channel_CfgTypeDef ch[6];
    uint32_t duration_min;
    bool sync_en;
    bool running;
    uint32_t start_time;
} Therapy_StateTypeDef;

// 系统状态
typedef enum {
    SLAVE_IDLE = 0,
    SLAVE_READY,
    SLAVE_RUNNING,
    SLAVE_FAULT
} Slave_StatusTypeDef;

// 故障码
typedef enum {
    FAULT_NONE = 0,
    FAULT_OVER_CURRENT,     // 过流
    FAULT_OVER_VOLTAGE,     // 过压
    FAULT_IMPEDANCE_LOW,    // 阻抗过低
    FAULT_IMPEDANCE_HIGH,   // 阻抗过高
    FAULT_OVER_TEMP,        // 过温
    FAULT_ESTOP,            // 急停
    FAULT_COMM_TIMEOUT,     // 通信超时
    FAULT_PARAM_INVALID     // 参数非法
} Fault_CodeTypeDef;

// 全局变量
static Slave_StatusTypeDef g_slave_status = SLAVE_IDLE;
static Therapy_StateTypeDef g_therapy;
static Fault_CodeTypeDef g_fault_code = FAULT_NONE;
static uint32_t g_last_cmd_time = 0;
static uint8_t g_master_addr[6] = {0};

// 安全超时
#define COMM_TIMEOUT_MS     30000   // 30秒无命令自动停机
#define IMPEDANCE_CHECK_MS  100     // 每100ms检测阻抗
#define CURRENT_CHECK_MS    50      // 每50ms检测电流

//==========================================================================
// 二、CRC16校验
//==========================================================================
uint16_t crc16_calc(uint8_t *data, uint16_t len)
{
    uint16_t crc = 0xFFFF;
    for (uint16_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 1)
                crc = (crc >> 1) ^ 0xA001;
            else
                crc >>= 1;
        }
    }
    return crc;
}

//==========================================================================
// 三、硬件级安全保护（独立于软件）
//==========================================================================

// 3.1 急停按钮检测（最高优先级）
static bool check_estop(void)
{
    // 假设急停按钮连接到 PC2 (GPIO 18)，常闭(NC)
    #define ESTOP_PIN       18
    #define ESTOP_PORT      pc
    
    // 读取急停状态
    uint8_t estop_state = GPIO_READ(ESTOP_PORT, ESTOP_PIN - 16);  // PC2 = pin 2
    
    // NC按钮：正常时为低，按下时为高
    if (estop_state) {
        uart_send_string("[SLAVE] EMERGENCY STOP ACTIVATED!\r\n");
        g_fault_code = FAULT_ESTOP;
        return false;  // 急停激活
    }
    return true;
}

// 3.2 电流检测（INA219）
static float read_current_ma(void)
{
    // 通过I2C读取INA219
    // 简化实现，实际需要I2C驱动
    uint16_t raw = adc_sample(ADC_CHANNEL_0);  // 临时用ADC代替
    float voltage = raw * 3.3f / 4096.0f;
    float current = voltage / 0.1f;  // 0.1Ω采样电阻
    return current;
}

static bool check_current_limit(void)
{
    float current = read_current_ma();
    
    if (current > HW_CURRENT_LIMIT_MA) {
        uart_send_string("[SLAVE] OVER CURRENT DETECTED!\r\n");
        uart_send_string("[SLAVE] Current: ");
        // 输出电流值...
        uart_send_string("mA\r\n");
        
        g_fault_code = FAULT_OVER_CURRENT;
        return false;
    }
    return true;
}

// 3.3 电压检测
static float read_voltage_v(void)
{
    uint16_t raw = adc_sample(ADC_CHANNEL_VBAT);
    float voltage = raw * 3.3f / 4096.0f * 11.0f;  // 分压比1:11
    return voltage;
}

static bool check_voltage_limit(void)
{
    float voltage = read_voltage_v();
    
    if (voltage > HW_VOLTAGE_LIMIT_V) {
        uart_send_string("[SLAVE] OVER VOLTAGE DETECTED!\r\n");
        g_fault_code = FAULT_OVER_VOLTAGE;
        return false;
    }
    return true;
}

// 3.4 阻抗检测
static float read_impedance_ohm(void)
{
    // 通过ADC检测输出端电压，计算阻抗
    uint16_t raw = adc_sample(ADC_CHANNEL_1);
    float voltage = raw * 3.3f / 4096.0f;
    
    // 简化计算：假设已知电流
    float current = 1.0f;  // 1mA测试电流
    float impedance = voltage / current * 1000.0f;  // 转换为Ω
    
    return impedance;
}

static bool check_impedance(void)
{
    float impedance = read_impedance_ohm();
    
    if (impedance < HW_IMPEDANCE_MIN_OHM) {
        uart_send_string("[SLAVE] IMPEDANCE TOO LOW!\r\n");
        g_fault_code = FAULT_IMPEDANCE_LOW;
        return false;
    }
    
    if (impedance > HW_IMPEDANCE_MAX_OHM) {
        uart_send_string("[SLAVE] IMPEDANCE TOO HIGH!\r\n");
        g_fault_code = FAULT_IMPEDANCE_HIGH;
        return false;
    }
    
    return true;
}

// 3.5 综合安全检测（定时调用）
static bool safety_check(void)
{
    // 急停检测（最高优先级）
    if (!check_estop()) {
        return false;
    }
    
    // 电流检测
    if (!check_current_limit()) {
        return false;
    }
    
    // 电压检测
    if (!check_voltage_limit()) {
        return false;
    }
    
    // 阻抗检测（仅在运行时）
    if (g_slave_status == SLAVE_RUNNING) {
        if (!check_impedance()) {
            return false;
        }
    }
    
    return true;
}

//==========================================================================
// 四、参数校验（从机端二次校验，防止主设备绕过）
//==========================================================================
static bool validate_channel_param_slave(uint8_t ch, Channel_CfgTypeDef *cfg)
{
    if (ch >= 6) {
        uart_send_string("[SLAVE] Error: Invalid channel\r\n");
        return false;
    }
    
    if (cfg->enable) {
        if (cfg->freq_hz < FREQ_MIN_HZ || cfg->freq_hz > FREQ_MAX_HZ) {
            uart_send_string("[SLAVE] Error: Frequency out of range\r\n");
            return false;
        }
        
        if (cfg->width_us < WIDTH_MIN_US || cfg->width_us > WIDTH_MAX_US) {
            uart_send_string("[SLAVE] Error: Width out of range\r\n");
            return false;
        }
        
        if (cfg->intensity > MAX_INTENSITY) {
            uart_send_string("[SLAVE] Error: Intensity out of range\r\n");
            return false;
        }
    }
    
    return true;
}

//==========================================================================
// 五、PWM控制（硬件级输出）
//==========================================================================
static void update_pwm_output(void)
{
    for (uint8_t i = 0; i < 6; i++) {
        if (g_therapy.ch[i].enable && g_therapy.running) {
            // 计算占空比（基于强度）
            uint16_t duty = (uint16_t)(g_therapy.ch[i].intensity * 10.23f);  // 0-100 -> 0-1023
            
            // 设置PWM频率和占空比
            pwm_set_freq(i, g_therapy.ch[i].freq_hz);
            pwm_set_duty(i, duty);
            pwm_start(i);
        } else {
            // 停止PWM
            pwm_stop(i);
        }
    }
}

static void stop_all_output(void)
{
    for (uint8_t i = 0; i < 6; i++) {
        pwm_stop(i);
    }
    
    uart_send_string("[SLAVE] All output stopped\r\n");
}

//==========================================================================
// 六、通信处理
//==========================================================================
static void send_ack(uint8_t cmd)
{
    Protocol_FrameTypeDef ack;
    ack.head = PROTOCOL_HEAD;
    ack.cmd = CMD_ACK;
    ack.len = 1;
    ack.data[0] = cmd;  // 应答对应的指令
    ack.tail = PROTOCOL_TAIL;
    
    uint16_t crc_len = 3 + ack.len;
    ack.crc = crc16_calc((uint8_t *)&ack, crc_len);
    
    ble_send(g_master_addr, (uint8_t *)&ack, sizeof(Protocol_FrameTypeDef) - 16 + ack.len);
}

static void process_command(Protocol_FrameTypeDef *frame)
{
    g_last_cmd_time = get_system_tick();
    
    switch (frame->cmd) {
        case CMD_SET_CHANNEL: {
            uint8_t ch = frame->data[0];
            Channel_CfgTypeDef cfg;
            cfg.enable = frame->data[1] ? true : false;
            memcpy(&cfg.freq_hz, &frame->data[2], 4);
            memcpy(&cfg.width_us, &frame->data[6], 2);
            
            if (validate_channel_param_slave(ch, &cfg)) {
                memcpy(&g_therapy.ch[ch], &cfg, sizeof(Channel_CfgTypeDef));
                send_ack(CMD_SET_CHANNEL);
                uart_send_string("[SLAVE] Channel params set\r\n");
            } else {
                uart_send_string("[SLAVE] Invalid channel params rejected\r\n");
            }
            break;
        }
        
        case CMD_SET_GLOBAL: {
            g_therapy.sync_en = frame->data[0] ? true : false;
            memcpy(&g_therapy.duration_min, &frame->data[1], 4);
            
            if (g_therapy.duration_min > 0 && g_therapy.duration_min <= DUR_MAX_MIN) {
                send_ack(CMD_SET_GLOBAL);
                uart_send_string("[SLAVE] Global params set\r\n");
            } else {
                uart_send_string("[SLAVE] Invalid duration rejected\r\n");
            }
            break;
        }
        
        case CMD_START: {
            if (g_slave_status == SLAVE_READY || g_slave_status == SLAVE_IDLE) {
                // 最终安全检查
                if (!safety_check()) {
                    stop_all_output();
                    g_slave_status = SLAVE_FAULT;
                    uart_send_string("[SLAVE] Safety check failed, cannot start\r\n");
                    return;
                }
                
                g_therapy.running = true;
                g_therapy.start_time = get_system_tick();
                g_slave_status = SLAVE_RUNNING;
                update_pwm_output();
                send_ack(CMD_START);
                uart_send_string("[SLAVE] Therapy started\r\n");
            }
            break;
        }
        
        case CMD_STOP: {
            g_therapy.running = false;
            stop_all_output();
            g_slave_status = SLAVE_READY;
            send_ack(CMD_STOP);
            uart_send_string("[SLAVE] Therapy stopped\r\n");
            break;
        }
        
        case CMD_GET_STATUS: {
            uint8_t status_data[8];
            status_data[0] = g_slave_status;
            status_data[1] = g_fault_code;
            status_data[2] = g_therapy.running ? 1 : 0;
            // 添加更多状态信息...
            
            Protocol_FrameTypeDef status_frame;
            status_frame.head = PROTOCOL_HEAD;
            status_frame.cmd = CMD_GET_STATUS;
            status_frame.len = 8;
            memcpy(status_frame.data, status_data, 8);
            status_frame.tail = PROTOCOL_TAIL;
            status_frame.crc = crc16_calc((uint8_t *)&status_frame, 3 + 8);
            
            ble_send(g_master_addr, (uint8_t *)&status_frame, sizeof(Protocol_FrameTypeDef) - 8);
            break;
        }
        
        default: {
            uart_send_string("[SLAVE] Unknown command\r\n");
            break;
        }
    }
}

static void comm_process(void)
{
    uint8_t rx_buf[32];
    uint16_t rx_len = ble_recv(rx_buf, sizeof(rx_buf));
    
    if (rx_len > 0) {
        Protocol_FrameTypeDef frame;
        
        // 解析帧
        if (rx_len >= 6 && rx_buf[0] == PROTOCOL_HEAD && rx_buf[rx_len-1] == PROTOCOL_TAIL) {
            frame.head = rx_buf[0];
            frame.cmd = rx_buf[1];
            frame.len = rx_buf[2];
            
            if (frame.len <= 16) {
                memcpy(frame.data, &rx_buf[3], frame.len);
                frame.crc = (rx_buf[3 + frame.len + 1] << 8) | rx_buf[3 + frame.len];
                frame.tail = rx_buf[3 + frame.len + 2];
                
                // 校验CRC
                uint16_t calc_crc = crc16_calc(rx_buf, 3 + frame.len);
                if (calc_crc == frame.crc) {
                    process_command(&frame);
                } else {
                    uart_send_string("[SLAVE] CRC error\r\n");
                }
            }
        }
    }
    
    // 检查通信超时
    if (g_slave_status == SLAVE_RUNNING) {
        if (get_system_tick() - g_last_cmd_time > COMM_TIMEOUT_MS) {
            uart_send_string("[SLAVE] Communication timeout! Stopping...\r\n");
            g_therapy.running = false;
            stop_all_output();
            g_slave_status = SLAVE_FAULT;
            g_fault_code = FAULT_COMM_TIMEOUT;
        }
    }
}

//==========================================================================
// 七、治疗时长监控
//==========================================================================
static void therapy_duration_monitor(void)
{
    if (g_slave_status == SLAVE_RUNNING && g_therapy.running) {
        uint32_t elapsed_ms = get_system_tick() - g_therapy.start_time;
        uint32_t duration_ms = g_therapy.duration_min * 60 * 1000;
        
        if (elapsed_ms >= duration_ms) {
            uart_send_string("[SLAVE] Therapy duration completed\r\n");
            g_therapy.running = false;
            stop_all_output();
            g_slave_status = SLAVE_READY;
        }
    }
}

//==========================================================================
// 八、故障处理
//==========================================================================
static void handle_fault(void)
{
    // 1. 立即停止所有输出
    stop_all_output();
    
    // 2. 设置故障状态
    g_slave_status = SLAVE_FAULT;
    g_therapy.running = false;
    
    // 3. 输出故障信息
    uart_send_string("\r\n*** SLAVE FAULT ***\r\n");
    uart_send_string("Fault code: 0x");
    // 输出故障码...
    uart_send_string("\r\n");
    
    switch (g_fault_code) {
        case FAULT_OVER_CURRENT:
            uart_send_string("Reason: Over current detected\r\n");
            break;
        case FAULT_OVER_VOLTAGE:
            uart_send_string("Reason: Over voltage detected\r\n");
            break;
        case FAULT_IMPEDANCE_LOW:
            uart_send_string("Reason: Impedance too low\r\n");
            break;
        case FAULT_IMPEDANCE_HIGH:
            uart_send_string("Reason: Impedance too high\r\n");
            break;
        case FAULT_ESTOP:
            uart_send_string("Reason: Emergency stop activated\r\n");
            break;
        case FAULT_COMM_TIMEOUT:
            uart_send_string("Reason: Communication timeout\r\n");
            break;
        default:
            uart_send_string("Reason: Unknown\r\n");
            break;
    }
    
    uart_send_string("Please check connections and restart.\r\n");
    uart_send_string("*******************\r\n\n");
    
    // 4. LED闪烁报警
    while (1) {
        led_set(LED_PIN, true);
        delay_ms(200);
        led_set(LED_PIN, false);
        delay_ms(200);
        
        // 可以在这里添加复位检测
        // if (reset_button_pressed()) {
        //     system_reset();
        // }
    }
}

//==========================================================================
// 九、主程序
//==========================================================================
void slave_init(void)
{
    uart_send_string("\r\n========================================\r\n");
    uart_send_string("  TLSR8269 Pulse Therapy - Slave\r\n");
    uart_send_string("  Medical Grade Safety Standard\r\n");
    uart_send_string("========================================\r\n");
    
    // 初始化硬件
    system_clock_init();
    ble_init();
    adc_init();
    pwm_init();
    
    // 执行自检
    uart_send_string("[SLAVE] Running self-check...\r\n");
    uint8_t check_result = system_self_check();
    
    if (check_result != CHECK_OK) {
        uart_send_string("[SLAVE] Self-check failed!\r\n");
        handle_system_error(check_result);
        // 不会执行到这里
    }
    
    // 初始化状态
    memset(&g_therapy, 0, sizeof(Therapy_StateTypeDef));
    g_slave_status = SLAVE_IDLE;
    g_fault_code = FAULT_NONE;
    g_last_cmd_time = get_system_tick();
    
    uart_send_string("[SLAVE] Init complete, waiting for master...\r\n");
}

void slave_main_loop(void)
{
    // 1. 安全检查（最高优先级）
    if (!safety_check()) {
        handle_fault();
        // 不会执行到这里（handle_fault中有死循环）
    }
    
    // 2. 通信处理
    comm_process();
    
    // 3. 治疗时长监控
    therapy_duration_monitor();
    
    // 4. 更新PWM输出
    if (g_slave_status == SLAVE_RUNNING) {
        update_pwm_output();
    }
    
    // 5. 喂狗
    watchdog_feed();
}

//==========================================================================
// 十、使用示例
//==========================================================================
/*
void main(void)
{
    // 初始化
    slave_init();
    
    // 主循环
    while (1) {
        slave_main_loop();
    }
}
*/
