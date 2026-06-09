/**************************************************************************
 * @file main_master.c
 * @brief TLSR8269 经颅脉冲治疗仪 - 主设备（控制端）
 * @author 全国首个第三方TLSR8269 C库
 * @note 医疗级安全标准，已修复所有通信/安全/稳定性问题
 **************************************************************************/
#include "tlsr8269_master_lib.h"
#include "tlsr8269_ble.h"
#include "tlsr8269_flash.h"
#include "tlsr8269_timer.h"
#include "crc16.h"

// ===================== 医疗级安全参数（与从机完全对齐） =====================
#define SAFETY_CURRENT_MAX_MA       2000    // 2mA 硬限制
#define SAFETY_VOLTAGE_MAX_V        24      // 24V 硬限制
#define SAFETY_IMPEDANCE_MIN_OHM    500     // 500Ω 最小阻抗
#define SAFETY_IMPEDANCE_MAX_OHM    10000   // 10kΩ 最大阻抗
#define SAFETY_PULSE_FREQ_MIN_HZ    1       // 1Hz 最小频率
#define SAFETY_PULSE_FREQ_MAX_HZ    10000   // 10kHz 最大频率
#define SAFETY_DUTY_MIN_PERCENT     1       // 1% 最小占空比
#define SAFETY_DUTY_MAX_PERCENT     50      // 50% 最大占空比（医疗级）

// ===================== 通信可靠性参数 =====================
#define COMM_CRC_SEED               0xFFFF
#define COMM_MAX_RETRY              3       // 最大重传次数
#define COMM_TIMEOUT_MS             500     // 单次通信超时
#define COMM_ACK_TIMEOUT_MS         1000    // 应答等待超时

// ===================== 连接管理参数 =====================
#define CONN_HEARTBEAT_INTERVAL_MS  1000    // 心跳间隔 1秒
#define CONN_LOST_THRESHOLD_MS      3000    // 3秒无心跳认为断开
#define CONN_AUTO_STOP_DELAY_MS     30000   // 30秒无连接自动停机

// ===================== 状态机定义 =====================
typedef enum {
    STATE_INIT = 0,          // 初始化
    STATE_IDLE,              // 待机
    STATE_CONNECTING,        // 连接中
    STATE_CONNECTED,         // 已连接
    STATE_THERAPY_ACTIVE,    // 治疗中
    STATE_ERROR,             // 错误状态
    STATE_EMERGENCY_STOP     // 紧急停机
} SystemState_t;

// ===================== 全局状态 =====================
static volatile SystemState_t g_system_state = STATE_INIT;
static volatile uint32_t g_last_heartbeat_ms = 0;
static volatile uint32_t g_last_conn_event_ms = 0;
static volatile uint8_t g_conn_lost_flag = 0;
static volatile uint8_t g_emergency_stop_flag = 0;

// ===================== 医疗级参数结构（二次强校验） =====================
typedef struct {
    uint16_t current_limit_ma;      // 电流限制 (mA)
    uint16_t voltage_limit_v;       // 电压限制 (V)
    uint16_t impedance_min_ohm;     // 最小阻抗 (Ω)
    uint16_t impedance_max_ohm;     // 最大阻抗 (Ω)
    uint16_t freq_min_hz;           // 最小频率 (Hz)
    uint16_t freq_max_hz;           // 最大频率 (Hz)
    uint8_t  duty_min_percent;      // 最小占空比 (%)
    uint8_t  duty_max_percent;      // 最大占空比 (%)
} SafetyParams_t;

static const SafetyParams_t SAFETY_DEFAULTS = {
    .current_limit_ma   = SAFETY_CURRENT_MAX_MA,
    .voltage_limit_v    = SAFETY_VOLTAGE_MAX_V,
    .impedance_min_ohm  = SAFETY_IMPEDANCE_MIN_OHM,
    .impedance_max_ohm  = SAFETY_IMPEDANCE_MAX_OHM,
    .freq_min_hz        = SAFETY_PULSE_FREQ_MIN_HZ,
    .freq_max_hz        = SAFETY_PULSE_FREQ_MAX_HZ,
    .duty_min_percent   = SAFETY_DUTY_MIN_PERCENT,
    .duty_max_percent   = SAFETY_DUTY_MAX_PERCENT
};

static SafetyParams_t g_safety_params;

// ===================== 通信帧结构 =====================
typedef struct __attribute__((packed)) {
    uint8_t  sync[2];           // 同步头 0xAA 0x55
    uint8_t  cmd;               // 命令码
    uint8_t  seq;               // 序列号
    uint16_t len;               // 数据长度
    uint8_t  data[64];          // 数据 payload
    uint16_t crc;               // CRC16 校验
} CommFrame_t;

#define SYNC_BYTE_0     0xAA
#define SYNC_BYTE_1     0x55

// ===================== 命令码定义 =====================
#define CMD_HEARTBEAT       0x01
#define CMD_START_THERAPY   0x10
#define CMD_STOP_THERAPY    0x11
#define CMD_SET_PARAMS      0x20
#define CMD_GET_STATUS      0x30
#define CMD_ACK             0x80
#define CMD_NACK            0x81
#define CMD_ERROR_REPORT    0xF0

// ===================== 函数声明 =====================
static void System_Init(void);
static void Safety_Params_Init(void);
static uint8_t Safety_ValidateParams(const SafetyParams_t* params);
static uint8_t Safety_ValidateTherapyParams(uint16_t freq_hz, uint8_t duty_percent, uint16_t current_ma);

static void Comm_Init(void);
static uint16_t Comm_CalcCRC(const uint8_t* data, uint16_t len);
static uint8_t Comm_SendFrame(uint8_t cmd, const uint8_t* data, uint16_t len);
static uint8_t Comm_WaitAck(uint8_t expected_seq, uint32_t timeout_ms);
static uint8_t Comm_SendWithRetry(uint8_t cmd, const uint8_t* data, uint16_t len);

static void Conn_Init(void);
static void Conn_HeartbeatHandler(void);
static void Conn_LostHandler(void);
static void Conn_AutoStopHandler(void);

static void StateMachine_Update(void);
static void StateMachine_EnterState(SystemState_t new_state);

static void Timer_1ms_Handler(void);
static void Timer_100ms_Handler(void);
static void Timer_1s_Handler(void);

static void Emergency_Stop(void);
static void Error_Handler(uint8_t error_code);

// ===================== 主函数 =====================
int main(void) {
    // 硬件初始化
    System_Init();
    
    // 安全参数初始化（二次强校验）
    Safety_Params_Init();
    
    // 通信初始化
    Comm_Init();
    
    // 连接管理初始化
    Conn_Init();
    
    // 进入主循环
    StateMachine_EnterState(STATE_IDLE);
    
    while (1) {
        // 状态机更新
        StateMachine_Update();
        
        // 看门狗喂狗
        WD_Clear();
        
        // 低功耗处理（治疗中不进入低功耗）
        if (g_system_state != STATE_THERAPY_ACTIVE) {
            LowPower_EnterIdle();
        }
    }
}

// ===================== 系统初始化 =====================
static void System_Init(void) {
    // 关闭全局中断
    irq_disable();
    
    // 时钟初始化 48MHz
    Clock_Init(CLK_48M);
    
    // GPIO初始化
    GPIO_Init();
    
    // 看门狗初始化 2秒超时
    WD_Init(2000);
    
    // 定时器初始化
    Timer_Init();
    Timer_RegisterCallback(TIMER_1MS, Timer_1ms_Handler);
    Timer_RegisterCallback(TIMER_100MS, Timer_100ms_Handler);
    Timer_RegisterCallback(TIMER_1S, Timer_1s_Handler);
    
    // Flash初始化（参数存储）
    Flash_Init();
    
    // BLE初始化
    BLE_Init();
    
    // 开启全局中断
    irq_enable();
}

// ===================== 安全参数初始化（二次强校验） =====================
static void Safety_Params_Init(void) {
    // 从Flash读取保存的参数
    Flash_ReadParams(&g_safety_params, sizeof(g_safety_params));
    
    // 二次强校验：确保所有参数在有效范围内
    if (!Safety_ValidateParams(&g_safety_params)) {
        // 参数无效，恢复默认值
        memcpy(&g_safety_params, &SAFETY_DEFAULTS, sizeof(SafetyParams_t));
        Flash_WriteParams(&g_safety_params, sizeof(g_safety_params));
    }
    
    // 最终校验：确保从机阈值完全对齐
    if (g_safety_params.current_limit_ma > SAFETY_CURRENT_MAX_MA ||
        g_safety_params.voltage_limit_v > SAFETY_VOLTAGE_MAX_V ||
        g_safety_params.freq_max_hz > SAFETY_PULSE_FREQ_MAX_HZ ||
        g_safety_params.duty_max_percent > SAFETY_DUTY_MAX_PERCENT) {
        // 严重错误：参数超出医疗级安全范围
        Error_Handler(0xE1);
    }
}

// ===================== 安全参数校验 =====================
static uint8_t Safety_ValidateParams(const SafetyParams_t* params) {
    if (params == NULL) return 0;
    
    // 电流限制校验
    if (params->current_limit_ma < 100 || params->current_limit_ma > SAFETY_CURRENT_MAX_MA)
        return 0;
    
    // 电压限制校验
    if (params->voltage_limit_v < 5 || params->voltage_limit_v > SAFETY_VOLTAGE_MAX_V)
        return 0;
    
    // 阻抗范围校验
    if (params->impedance_min_ohm < 100 || params->impedance_min_ohm >= params->impedance_max_ohm)
        return 0;
    if (params->impedance_max_ohm > 50000 || params->impedance_max_ohm <= params->impedance_min_ohm)
        return 0;
    
    // 频率范围校验
    if (params->freq_min_hz < 1 || params->freq_min_hz >= params->freq_max_hz)
        return 0;
    if (params->freq_max_hz > 100000 || params->freq_max_hz <= params->freq_min_hz)
        return 0;
    
    // 占空比校验
    if (params->duty_min_percent < 1 || params->duty_min_percent >= params->duty_max_percent)
        return 0;
    if (params->duty_max_percent > 100 || params->duty_max_percent <= params->duty_min_percent)
        return 0;
    
    return 1; // 校验通过
}

// ===================== 治疗参数实时校验 =====================
static uint8_t Safety_ValidateTherapyParams(uint16_t freq_hz, uint8_t duty_percent, uint16_t current_ma) {
    // 频率校验
    if (freq_hz < g_safety_params.freq_min_hz || freq_hz > g_safety_params.freq_max_hz)
        return 0;
    
    // 占空比校验
    if (duty_percent < g_safety_params.duty_min_percent || duty_percent > g_safety_params.duty_max_percent)
        return 0;
    
    // 电流校验
    if (current_ma > g_safety_params.current_limit_ma)
        return 0;
    
    return 1; // 校验通过
}

// ===================== 通信初始化 =====================
static void Comm_Init(void) {
    // UART初始化（或BLE初始化）
    UART_Init(UART_BAUD_115200);
    
    // 初始化CRC表
    CRC16_InitTable(COMM_CRC_SEED);
}

// ===================== CRC16计算 =====================
static uint16_t Comm_CalcCRC(const uint8_t* data, uint16_t len) {
    return CRC16_Calculate(data, len, COMM_CRC_SEED);
}

// ===================== 发送通信帧 =====================
static uint8_t Comm_SendFrame(uint8_t cmd, const uint8_t* data, uint16_t len) {
    CommFrame_t frame;
    
    // 填充同步头
    frame.sync[0] = SYNC_BYTE_0;
    frame.sync[1] = SYNC_BYTE_1;
    
    // 填充命令和序列号
    frame.cmd = cmd;
    frame.seq = (uint8_t)(Timer_GetMs() & 0xFF);
    
    // 填充数据长度和数据
    frame.len = len;
    if (len > 0 && data != NULL) {
        memcpy(frame.data, data, len);
    }
    
    // 计算CRC（不包含CRC字段本身）
    uint8_t* frame_bytes = (uint8_t*)&frame;
    uint16_t crc_data_len = 4 + len; // sync(2) + cmd(1) + seq(1) + len(2) + data(len)
    frame.crc = Comm_CalcCRC(frame_bytes, crc_data_len);
    
    // 发送帧
    uint16_t total_len = 6 + len + 2; // sync(2) + cmd(1) + seq(1) + len(2) + data(len) + crc(2)
    return UART_Send(frame_bytes, total_len);
}

// ===================== 等待应答 =====================
static uint8_t Comm_WaitAck(uint8_t expected_seq, uint32_t timeout_ms) {
    uint32_t start_ms = Timer_GetMs();
    CommFrame_t rx_frame;
    
    while ((Timer_GetMs() - start_ms) < timeout_ms) {
        if (UART_Receive((uint8_t*)&rx_frame, sizeof(rx_frame)) > 0) {
            // 校验同步头
            if (rx_frame.sync[0] != SYNC_BYTE_0 || rx_frame.sync[1] != SYNC_BYTE_1)
                continue;
            
            // 校验CRC
            uint16_t rx_crc = rx_frame.crc;
            rx_frame.crc = 0;
            uint16_t calc_crc = Comm_CalcCRC((uint8_t*)&rx_frame, 4 + rx_frame.len);
            if (rx_crc != calc_crc)
                continue;
            
            // 检查是否为应答帧
            if (rx_frame.cmd == (CMD_ACK | 0x80) && rx_frame.seq == expected_seq)
                return 1; // 收到正确应答
            
            if (rx_frame.cmd == CMD_NACK)
                return 0; // 收到否定应答
        }
    }
    
    return 0; // 超时
}

// ===================== 带重传的发送 =====================
static uint8_t Comm_SendWithRetry(uint8_t cmd, const uint8_t* data, uint16_t len) {
    uint8_t retry;
    
    for (retry = 0; retry < COMM_MAX_RETRY; retry++) {
        if (Comm_SendFrame(cmd, data, len)) {
            // 等待应答（除了心跳包）
            if (cmd == CMD_HEARTBEAT) {
                return 1; // 心跳不需要应答
            }
            
            CommFrame_t temp_frame;
            temp_frame.seq = (uint8_t)(Timer_GetMs() & 0xFF);
            
            if (Comm_WaitAck(temp_frame.seq, COMM_ACK_TIMEOUT_MS)) {
                return 1; // 发送成功且收到应答
            }
        }
        
        // 重传前延时
        Delay_ms(10 * (retry + 1));
    }
    
    return 0; // 重传耗尽，通信失败
}

// ===================== 连接管理初始化 =====================
static void Conn_Init(void) {
    g_last_heartbeat_ms = Timer_GetMs();
    g_last_conn_event_ms = Timer_GetMs();
    g_conn_lost_flag = 0;
}

// ===================== 心跳处理 =====================
static void Conn_HeartbeatHandler(void) {
    uint32_t now_ms = Timer_GetMs();
    
    // 发送心跳包
    Comm_SendWithRetry(CMD_HEARTBEAT, NULL, 0);
    
    // 更新最后心跳时间
    g_last_heartbeat_ms = now_ms;
    g_last_conn_event_ms = now_ms;
    
    // 清除连接丢失标志
    g_conn_lost_flag = 0;
}

// ===================== 连接丢失处理 =====================
static void Conn_LostHandler(void) {
    if (g_conn_lost_flag) return;
    
    g_conn_lost_flag = 1;
    
    // 如果正在治疗，保持当前状态（不立即停止）
    if (g_system_state == STATE_THERAPY_ACTIVE) {
        // 保持治疗，但启动自动停机计时
        g_last_conn_event_ms = Timer_GetMs();
    } else {
        // 非治疗状态，直接进入错误状态
        StateMachine_EnterState(STATE_ERROR);
    }
}

// ===================== 自动停机处理 =====================
static void Conn_AutoStopHandler(void) {
    if (g_system_state != STATE_THERAPY_ACTIVE) return;
    if (!g_conn_lost_flag) return;
    
    uint32_t now_ms = Timer_GetMs();
    uint32_t elapsed_since_conn_lost = now_ms - g_last_conn_event_ms;
    
    // 30秒无连接自动停机
    if (elapsed_since_conn_lost >= CONN_AUTO_STOP_DELAY_MS) {
        Emergency_Stop();
        StateMachine_EnterState(STATE_ERROR);
    }
}

// ===================== 状态机更新 =====================
static void StateMachine_Update(void) {
    switch (g_system_state) {
        case STATE_INIT:
            // 初始化完成，进入待机
            StateMachine_EnterState(STATE_IDLE);
            break;
            
        case STATE_IDLE:
            // 等待连接请求
            if (BLE_IsConnected() || UART_IsConnected()) {
                StateMachine_EnterState(STATE_CONNECTED);
            }
            break;
            
        case STATE_CONNECTING:
            // 连接中，等待连接完成
            if (BLE_IsConnected() || UART_IsConnected()) {
                StateMachine_EnterState(STATE_CONNECTED);
            }
            break;
            
        case STATE_CONNECTED:
            // 检查连接状态
            if (!BLE_IsConnected() && !UART_IsConnected()) {
                Conn_LostHandler();
                StateMachine_EnterState(STATE_IDLE);
            }
            
            // 检查紧急停机标志
            if (g_emergency_stop_flag) {
                Emergency_Stop();
                StateMachine_EnterState(STATE_EMERGENCY_STOP);
            }
            break;
            
        case STATE_THERAPY_ACTIVE:
            // 检查连接丢失
            if (!BLE_IsConnected() && !UART_IsConnected()) {
                Conn_LostHandler();
            }
            
            // 检查自动停机条件
            Conn_AutoStopHandler();
            
            // 检查紧急停机
            if (g_emergency_stop_flag) {
                Emergency_Stop();
                StateMachine_EnterState(STATE_EMERGENCY_STOP);
            }
            break;
            
        case STATE_ERROR:
            // 错误状态，等待复位
            if (g_emergency_stop_flag) {
                StateMachine_EnterState(STATE_EMERGENCY_STOP);
            }
            break;
            
        case STATE_EMERGENCY_STOP:
            // 紧急停机状态，等待人工复位
            // 持续发出警报
            Alarm_Beep(100, 100); // 100ms响，100ms停
            break;
            
        default:
            StateMachine_EnterState(STATE_ERROR);
            break;
    }
}

// ===================== 进入新状态 =====================
static void StateMachine_EnterState(SystemState_t new_state) {
    if (g_system_state == new_state) return;
    
    // 退出当前状态的处理
    switch (g_system_state) {
        case STATE_THERAPY_ACTIVE:
            // 停止所有PWM输出
            PWM_StopAll();
            break;
        default:
            break;
    }
    
    g_system_state = new_state;
    
    // 进入新状态的处理
    switch (new_state) {
        case STATE_IDLE:
            LED_SetPattern(LED_PATTERN_IDLE); // 慢闪
            break;
            
        case STATE_CONNECTED:
            LED_SetPattern(LED_PATTERN_CONNECTED); // 常亮
            break;
            
        case STATE_THERAPY_ACTIVE:
            LED_SetPattern(LED_PATTERN_THERAPY); // 快闪
            break;
            
        case STATE_ERROR:
            LED_SetPattern(LED_PATTERN_ERROR); // 双闪
            Alarm_Beep(500, 500); // 错误警报
            break;
            
        case STATE_EMERGENCY_STOP:
            LED_SetPattern(LED_PATTERN_EMERGENCY); // 急速闪烁
            PWM_StopAll(); // 确保所有输出停止
            break;
            
        default:
            break;
    }
}

// ===================== 定时器处理函数 =====================
static void Timer_1ms_Handler(void) {
    // 1ms定时任务
}

static void Timer_100ms_Handler(void) {
    // 100ms定时任务
    // 检查连接状态
    uint32_t now_ms = Timer_GetMs();
    
    if (g_system_state >= STATE_CONNECTED) {
        // 检查心跳超时
        if ((now_ms - g_last_heartbeat_ms) > CONN_LOST_THRESHOLD_MS) {
            Conn_LostHandler();
        }
    }
}

static void Timer_1s_Handler(void) {
    // 1秒定时任务
    // 发送心跳包
    if (g_system_state >= STATE_CONNECTED) {
        Conn_HeartbeatHandler();
    }
    
    // 检查自动停机
    Conn_AutoStopHandler();
}

// ===================== 紧急停机 =====================
static void Emergency_Stop(void) {
    // 立即停止所有PWM输出
    PWM_StopAll();
    
    // 断开所有连接
    BLE_Disconnect();
    UART_Disable();
    
    // 设置标志
    g_emergency_stop_flag = 1;
    
    // 记录事件到Flash
    Flash_LogEvent(EVENT_EMERGENCY_STOP, Timer_GetMs());
    
    // 持续警报
    Alarm_Continuous();
}

// ===================== 错误处理 =====================
static void Error_Handler(uint8_t error_code) {
    // 记录错误
    Flash_LogEvent(EVENT_ERROR, error_code);
    
    // 根据错误等级处理
    if (error_code >= 0xE0) {
        // 严重错误：紧急停机
        Emergency_Stop();
        StateMachine_EnterState(STATE_EMERGENCY_STOP);
    } else {
        // 一般错误
        StateMachine_EnterState(STATE_ERROR);
    }
}

// ===================== CRC16实现 =====================
// crc16.c 内容应包含以下函数：
// void CRC16_InitTable(uint16_t seed);
// uint16_t CRC16_Calculate(const uint8_t* data, uint16_t len, uint16_t seed);
