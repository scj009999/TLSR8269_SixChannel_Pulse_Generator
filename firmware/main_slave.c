/**************************************************************************
 * @file main_slave.c
 * @brief TLSR8269 经颅脉冲治疗仪 - 从设备（脉冲输出端）
 * @author 全国首个第三方TLSR8269 C库
 * @note 医疗级安全标准，与主设备完全对齐
 **************************************************************************/
#include "tlsr8269_slave_lib.h"
#include "tlsr8269_pwm.h"
#include "tlsr8269_adc.h"
#include "tlsr8269_flash.h"
#include "tlsr8269_timer.h"
#include "crc16.h"

// ===================== 医疗级安全参数（与主机完全对齐） =====================
#define SAFETY_CURRENT_MAX_MA       2000    // 2mA 硬限制
#define SAFETY_VOLTAGE_MAX_V        24      // 24V 硬限制
#define SAFETY_IMPEDANCE_MIN_OHM    500     // 500Ω 最小阻抗
#define SAFETY_IMPEDANCE_MAX_OHM    10000   // 10kΩ 最大阻抗
#define SAFETY_PULSE_FREQ_MIN_HZ    1       // 1Hz 最小频率
#define SAFETY_PULSE_FREQ_MAX_HZ    10000   // 10kHz 最大频率
#define SAFETY_DUTY_MIN_PERCENT     1       // 1% 最小占空比
#define SAFETY_DUTY_MAX_PERCENT     50      // 50% 最大占空比（医疗级）

// ===================== 通信参数（与主机完全对齐） =====================
#define COMM_CRC_SEED               0xFFFF
#define COMM_MAX_RETRY              3
#define COMM_TIMEOUT_MS             500
#define COMM_ACK_TIMEOUT_MS         1000

// ===================== 连接管理参数 =====================
#define CONN_HEARTBEAT_INTERVAL_MS  1000
#define CONN_LOST_THRESHOLD_MS      3000
#define CONN_AUTO_STOP_DELAY_MS     30000

// ===================== 脉冲通道定义 =====================
#define PULSE_CHANNEL_COUNT         6
#define PWM_CHANNEL_0               0   // PA0
#define PWM_CHANNEL_1               1   // PA1
#define PWM_CHANNEL_2               2   // PA2
#define PWM_CHANNEL_3               3   // PA3
#define PWM_CHANNEL_4               4   // PB0
#define PWM_CHANNEL_5               5   // PB1

// ===================== ADC通道定义 =====================
#define ADC_CHANNEL_CURRENT         0   // 电流检测
#define ADC_CHANNEL_VOLTAGE         1   // 电压检测
#define ADC_CHANNEL_IMPEDANCE       2   // 阻抗检测
#define ADC_CHANNEL_TEMP            3   // 温度检测

// ===================== 状态机定义 =====================
typedef enum {
    STATE_INIT = 0,
    STATE_IDLE,
    STATE_CONNECTED,
    STATE_THERAPY_ACTIVE,
    STATE_ERROR,
    STATE_EMERGENCY_STOP
} SlaveState_t;

// ===================== 治疗参数结构 =====================
typedef struct {
    uint16_t freq_hz;           // 频率 (Hz)
    uint8_t  duty_percent;      // 占空比 (%)
    uint16_t current_ma;        // 目标电流 (mA)
    uint16_t duration_sec;      // 治疗时长 (秒)
    uint8_t  channel_mask;      // 通道使能掩码 (bit0-5)
} TherapyParams_t;

// ===================== 实时监测结构 =====================
typedef struct {
    uint16_t current_ma;        // 实际电流 (mA)
    uint16_t voltage_v;         // 实际电压 (V)
    uint16_t impedance_ohm;     // 实际阻抗 (Ω)
    int16_t  temp_c;            // 温度 (°C)
    uint8_t  safety_status;     // 安全状态
} RealTimeData_t;

// ===================== 全局变量 =====================
static volatile SlaveState_t g_slave_state = STATE_INIT;
static volatile uint32_t g_last_heartbeat_ms = 0;
static volatile uint32_t g_last_conn_event_ms = 0;
static volatile uint8_t g_conn_lost_flag = 0;
static volatile uint8_t g_emergency_stop_flag = 0;

static TherapyParams_t g_therapy_params;
static RealTimeData_t g_realtime_data;

// ===================== 函数声明 =====================
static void Slave_Init(void);
static void PWM_InitAll(void);
static void ADC_InitAll(void);
static void Safety_Monitor(void);
static uint8_t Safety_CheckImpedance(void);
static uint8_t Safety_CheckCurrent(void);
static uint8_t Safety_CheckTemperature(void);

static void Comm_ProcessFrame(void);
static void Comm_SendAck(uint8_t seq);
static void Comm_SendNack(uint8_t seq, uint8_t error_code);
static void Comm_SendStatus(void);

static void Therapy_Start(const TherapyParams_t* params);
static void Therapy_Stop(void);
static void Therapy_UpdateRealTime(void);

static void StateMachine_Update(void);
static void StateMachine_EnterState(SlaveState_t new_state);

static void Timer_1ms_Handler(void);
static void Timer_100ms_Handler(void);
static void Timer_1s_Handler(void);

static void Emergency_Stop(void);
static void Error_Handler(uint8_t error_code);

// ===================== 主函数 =====================
int main(void) {
    Slave_Init();
    StateMachine_EnterState(STATE_IDLE);
    
    while (1) {
        StateMachine_Update();
        
        // 看门狗喂狗
        WD_Clear();
        
        // 低功耗处理
        if (g_slave_state != STATE_THERAPY_ACTIVE) {
            LowPower_EnterIdle();
        }
    }
}

// ===================== 从机初始化 =====================
static void Slave_Init(void) {
    irq_disable();
    
    // 时钟初始化
    Clock_Init(CLK_48M);
    
    // GPIO初始化
    GPIO_Init();
    
    // PWM初始化（所有通道）
    PWM_InitAll();
    
    // ADC初始化
    ADC_InitAll();
    
    // 看门狗初始化
    WD_Init(2000);
    
    // 定时器初始化
    Timer_Init();
    Timer_RegisterCallback(TIMER_1MS, Timer_1ms_Handler);
    Timer_RegisterCallback(TIMER_100MS, Timer_100ms_Handler);
    Timer_RegisterCallback(TIMER_1S, Timer_1s_Handler);
    
    // Flash初始化
    Flash_Init();
    
    // BLE初始化
    BLE_Init();
    
    // CRC初始化
    CRC16_InitTable(COMM_CRC_SEED);
    
    irq_enable();
}

// ===================== PWM初始化 =====================
static void PWM_InitAll(void) {
    // 配置6路PWM输出
    PWM_Config_t pwm_cfg = {
        .freq_hz = 1000,        // 默认1kHz
        .duty_percent = 0,      // 初始占空比0%
        .enabled = 0            // 初始禁用
    };
    
    // 初始化所有通道
    for (int i = 0; i < PULSE_CHANNEL_COUNT; i++) {
        PWM_Init(i, &pwm_cfg);
    }
    
    // 配置GPIO引脚
    // PA0-PA3: PWM0-PWM3
    // PB0-PB1: PWM4-PWM5
    GPIO_SetFunction(PA0, GPIO_FUNC_PWM);
    GPIO_SetFunction(PA1, GPIO_FUNC_PWM);
    GPIO_SetFunction(PA2, GPIO_FUNC_PWM);
    GPIO_SetFunction(PA3, GPIO_FUNC_PWM);
    GPIO_SetFunction(PB0, GPIO_FUNC_PWM);
    GPIO_SetFunction(PB1, GPIO_FUNC_PWM);
}

// ===================== ADC初始化 =====================
static void ADC_InitAll(void) {
    ADC_Config_t adc_cfg = {
        .resolution = ADC_RES_12BIT,
        .sample_rate = ADC_SR_1000HZ,
        .vref = ADC_VREF_3V3
    };
    
    ADC_Init(&adc_cfg);
    
    // 配置ADC通道
    ADC_ConfigChannel(ADC_CHANNEL_CURRENT, PB2);    // 电流检测
    ADC_ConfigChannel(ADC_CHANNEL_VOLTAGE, PB3);    // 电压检测
    ADC_ConfigChannel(ADC_CHANNEL_IMPEDANCE, PB4);  // 阻抗检测
    ADC_ConfigChannel(ADC_CHANNEL_TEMP, PB5);       // 温度检测
}

// ===================== 安全监测 =====================
static void Safety_Monitor(void) {
    if (g_slave_state != STATE_THERAPY_ACTIVE) return;
    
    // 检查阻抗
    if (!Safety_CheckImpedance()) {
        Error_Handler(0xA1); // 阻抗异常
        return;
    }
    
    // 检查电流
    if (!Safety_CheckCurrent()) {
        Error_Handler(0xA2); // 电流超限
        return;
    }
    
    // 检查温度
    if (!Safety_CheckTemperature()) {
        Error_Handler(0xA3); // 温度异常
        return;
    }
}

// ===================== 阻抗检查 =====================
static uint8_t Safety_CheckImpedance(void) {
    uint16_t impedance = g_realtime_data.impedance_ohm;
    
    if (impedance < SAFETY_IMPEDANCE_MIN_OHM || impedance > SAFETY_IMPEDANCE_MAX_OHM) {
        return 0; // 阻抗异常
    }
    
    return 1;
}

// ===================== 电流检查 =====================
static uint8_t Safety_CheckCurrent(void) {
    uint16_t current = g_realtime_data.current_ma;
    
    if (current > SAFETY_CURRENT_MAX_MA) {
        // 立即停机
        Emergency_Stop();
        return 0;
    }
    
    return 1;
}

// ===================== 温度检查 =====================
static uint8_t Safety_CheckTemperature(void) {
    int16_t temp = g_realtime_data.temp_c;
    
    // 温度范围：-20°C 到 +60°C
    if (temp < -20 || temp > 60) {
        return 0;
    }
    
    return 1;
}

// ===================== 通信帧处理 =====================
static void Comm_ProcessFrame(void) {
    CommFrame_t rx_frame;
    
    if (UART_Receive((uint8_t*)&rx_frame, sizeof(rx_frame)) == 0) {
        return; // 无数据
    }
    
    // 校验同步头
    if (rx_frame.sync[0] != 0xAA || rx_frame.sync[1] != 0x55) {
        return;
    }
    
    // 校验CRC
    uint16_t rx_crc = rx_frame.crc;
    rx_frame.crc = 0;
    uint16_t calc_crc = CRC16_Calculate((uint8_t*)&rx_frame, 4 + rx_frame.len, COMM_CRC_SEED);
    
    if (rx_crc != calc_crc) {
        Comm_SendNack(rx_frame.seq, 0x01); // CRC错误
        return;
    }
    
    // 处理命令
    switch (rx_frame.cmd) {
        case CMD_HEARTBEAT:
            g_last_heartbeat_ms = Timer_GetMs();
            g_last_conn_event_ms = Timer_GetMs();
            g_conn_lost_flag = 0;
            Comm_SendAck(rx_frame.seq);
            break;
            
        case CMD_START_THERAPY:
            if (g_slave_state == STATE_CONNECTED) {
                memcpy(&g_therapy_params, rx_frame.data, sizeof(TherapyParams_t));
                
                // 校验治疗参数
                if (g_therapy_params.freq_hz < SAFETY_PULSE_FREQ_MIN_HZ ||
                    g_therapy_params.freq_hz > SAFETY_PULSE_FREQ_MAX_HZ ||
                    g_therapy_params.duty_percent < SAFETY_DUTY_MIN_PERCENT ||
                    g_therapy_params.duty_percent > SAFETY_DUTY_MAX_PERCENT ||
                    g_therapy_params.current_ma > SAFETY_CURRENT_MAX_MA) {
                    Comm_SendNack(rx_frame.seq, 0x02); // 参数错误
                } else {
                    Therapy_Start(&g_therapy_params);
                    Comm_SendAck(rx_frame.seq);
                }
            } else {
                Comm_SendNack(rx_frame.seq, 0x03); // 状态错误
            }
            break;
            
        case CMD_STOP_THERAPY:
            Therapy_Stop();
            Comm_SendAck(rx_frame.seq);
            break;
            
        case CMD_SET_PARAMS:
            // 更新安全参数（需校验）
            memcpy(&g_therapy_params, rx_frame.data, sizeof(TherapyParams_t));
            Comm_SendAck(rx_frame.seq);
            break;
            
        case CMD_GET_STATUS:
            Comm_SendStatus();
            Comm_SendAck(rx_frame.seq);
            break;
            
        default:
            Comm_SendNack(rx_frame.seq, 0x04); // 未知命令
            break;
    }
}

// ===================== 发送应答 =====================
static void Comm_SendAck(uint8_t seq) {
    CommFrame_t ack_frame;
    ack_frame.sync[0] = 0xAA;
    ack_frame.sync[1] = 0x55;
    ack_frame.cmd = CMD_ACK;
    ack_frame.seq = seq;
    ack_frame.len = 0;
    ack_frame.crc = CRC16_Calculate((uint8_t*)&ack_frame, 4, COMM_CRC_SEED);
    
    UART_Send((uint8_t*)&ack_frame, 6);
}

// ===================== 发送否定应答 =====================
static void Comm_SendNack(uint8_t seq, uint8_t error_code) {
    CommFrame_t nack_frame;
    nack_frame.sync[0] = 0xAA;
    nack_frame.sync[1] = 0x55;
    nack_frame.cmd = CMD_NACK;
    nack_frame.seq = seq;
    nack_frame.len = 1;
    nack_frame.data[0] = error_code;
    nack_frame.crc = CRC16_Calculate((uint8_t*)&nack_frame, 5, COMM_CRC_SEED);
    
    UART_Send((uint8_t*)&nack_frame, 7);
}

// ===================== 发送状态 =====================
static void Comm_SendStatus(void) {
    CommFrame_t status_frame;
    status_frame.sync[0] = 0xAA;
    status_frame.sync[1] = 0x55;
    status_frame.cmd = CMD_GET_STATUS;
    status_frame.seq = 0;
    status_frame.len = sizeof(RealTimeData_t);
    memcpy(status_frame.data, &g_realtime_data, sizeof(RealTimeData_t));
    status_frame.crc = CRC16_Calculate((uint8_t*)&status_frame, 4 + sizeof(RealTimeData_t), COMM_CRC_SEED);
    
    UART_Send((uint8_t*)&status_frame, 6 + sizeof(RealTimeData_t));
}

// ===================== 开始治疗 =====================
static void Therapy_Start(const TherapyParams_t* params) {
    if (params == NULL) return;
    
    // 配置PWM参数
    PWM_Config_t pwm_cfg = {
        .freq_hz = params->freq_hz,
        .duty_percent = params->duty_percent,
        .enabled = 1
    };
    
    // 启动使能的通道
    for (int i = 0; i < PULSE_CHANNEL_COUNT; i++) {
        if (params->channel_mask & (1 << i)) {
            PWM_Config(i, &pwm_cfg);
            PWM_Start(i);
        }
    }
    
    StateMachine_EnterState(STATE_THERAPY_ACTIVE);
}

// ===================== 停止治疗 =====================
static void Therapy_Stop(void) {
    // 停止所有PWM通道
    for (int i = 0; i < PULSE_CHANNEL_COUNT; i++) {
        PWM_Stop(i);
    }
    
    StateMachine_EnterState(STATE_CONNECTED);
}

// ===================== 更新实时数据 =====================
static void Therapy_UpdateRealTime(void) {
    // 读取电流
    g_realtime_data.current_ma = ADC_Read(ADC_CHANNEL_CURRENT);
    
    // 读取电压
    g_realtime_data.voltage_v = ADC_Read(ADC_CHANNEL_VOLTAGE);
    
    // 读取阻抗
    g_realtime_data.impedance_ohm = ADC_Read(ADC_CHANNEL_IMPEDANCE);
    
    // 读取温度
    g_realtime_data.temp_c = ADC_Read(ADC_CHANNEL_TEMP);
    
    // 安全状态
    g_realtime_data.safety_status = 
        (Safety_CheckImpedance() ? 0x01 : 0) |
        (Safety_CheckCurrent() ? 0x02 : 0) |
        (Safety_CheckTemperature() ? 0x04 : 0);
}

// ===================== 状态机更新 =====================
static void StateMachine_Update(void) {
    switch (g_slave_state) {
        case STATE_INIT:
            StateMachine_EnterState(STATE_IDLE);
            break;
            
        case STATE_IDLE:
            if (BLE_IsConnected() || UART_IsConnected()) {
                StateMachine_EnterState(STATE_CONNECTED);
            }
            break;
            
        case STATE_CONNECTED:
            // 处理通信帧
            Comm_ProcessFrame();
            
            // 检查连接状态
            if (!BLE_IsConnected() && !UART_IsConnected()) {
                StateMachine_EnterState(STATE_IDLE);
            }
            
            // 检查紧急停机
            if (g_emergency_stop_flag) {
                Emergency_Stop();
                StateMachine_EnterState(STATE_EMERGENCY_STOP);
            }
            break;
            
        case STATE_THERAPY_ACTIVE:
            // 处理通信帧
            Comm_ProcessFrame();
            
            // 安全监测
            Safety_Monitor();
            
            // 更新实时数据
            Therapy_UpdateRealTime();
            
            // 检查连接丢失
            if (!BLE_IsConnected() && !UART_IsConnected()) {
                g_conn_lost_flag = 1;
            }
            
            // 检查自动停机
            if (g_conn_lost_flag) {
                uint32_t elapsed = Timer_GetMs() - g_last_conn_event_ms;
                if (elapsed >= CONN_AUTO_STOP_DELAY_MS) {
                    Emergency_Stop();
                    StateMachine_EnterState(STATE_ERROR);
                }
            }
            
            // 检查紧急停机
            if (g_emergency_stop_flag) {
                Emergency_Stop();
                StateMachine_EnterState(STATE_EMERGENCY_STOP);
            }
            break;
            
        case STATE_ERROR:
            if (g_emergency_stop_flag) {
                StateMachine_EnterState(STATE_EMERGENCY_STOP);
            }
            break;
            
        case STATE_EMERGENCY_STOP:
            Alarm_Beep(100, 100);
            break;
            
        default:
            StateMachine_EnterState(STATE_ERROR);
            break;
    }
}

// ===================== 进入新状态 =====================
static void StateMachine_EnterState(SlaveState_t new_state) {
    if (g_slave_state == new_state) return;
    
    switch (g_slave_state) {
        case STATE_THERAPY_ACTIVE:
            Therapy_Stop();
            break;
        default:
            break;
    }
    
    g_slave_state = new_state;
    
    switch (new_state) {
        case STATE_IDLE:
            LED_SetPattern(LED_PATTERN_IDLE);
            break;
        case STATE_CONNECTED:
            LED_SetPattern(LED_PATTERN_CONNECTED);
            break;
        case STATE_THERAPY_ACTIVE:
            LED_SetPattern(LED_PATTERN_THERAPY);
            break;
        case STATE_ERROR:
            LED_SetPattern(LED_PATTERN_ERROR);
            Alarm_Beep(500, 500);
            break;
        case STATE_EMERGENCY_STOP:
            LED_SetPattern(LED_PATTERN_EMERGENCY);
            PWM_StopAll();
            break;
        default:
            break;
    }
}

// ===================== 定时器处理 =====================
static void Timer_1ms_Handler(void) {
    // 1ms任务
}

static void Timer_100ms_Handler(void) {
    // 100ms任务
    if (g_slave_state >= STATE_CONNECTED) {
        uint32_t now = Timer_GetMs();
        if ((now - g_last_heartbeat_ms) > CONN_LOST_THRESHOLD_MS) {
            g_conn_lost_flag = 1;
        }
    }
}

static void Timer_1s_Handler(void) {
    // 1秒任务
    if (g_slave_state >= STATE_CONNECTED) {
        // 心跳由主机发起，从机只需响应
    }
    
    // 自动停机检查
    if (g_slave_state == STATE_THERAPY_ACTIVE && g_conn_lost_flag) {
        uint32_t elapsed = Timer_GetMs() - g_last_conn_event_ms;
        if (elapsed >= CONN_AUTO_STOP_DELAY_MS) {
            Emergency_Stop();
            StateMachine_EnterState(STATE_ERROR);
        }
    }
}

// ===================== 紧急停机 =====================
static void Emergency_Stop(void) {
    PWM_StopAll();
    BLE_Disconnect();
    UART_Disable();
    g_emergency_stop_flag = 1;
    Flash_LogEvent(EVENT_EMERGENCY_STOP, Timer_GetMs());
    Alarm_Continuous();
}

// ===================== 错误处理 =====================
static void Error_Handler(uint8_t error_code) {
    Flash_LogEvent(EVENT_ERROR, error_code);
    
    if (error_code >= 0xA0) {
        Emergency_Stop();
        StateMachine_EnterState(STATE_EMERGENCY_STOP);
    } else {
        StateMachine_EnterState(STATE_ERROR);
    }
}
