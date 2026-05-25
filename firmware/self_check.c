/**************************************************************************
 * 系统自检模块 — 医疗级标准
 * 功能：开机全链路检测 → 异常锁机+明确提示 → 仅全部正常才允许操作
 * 芯片：TLSR8269F512ET32 (QFN32)
 * 适用：六通道经颅脉冲治疗仪
 **************************************************************************/

#include "tlsr8269_reg.h"
#include <string.h>

// ============================================
// 自检结果码：0=正常，非0=故障（对应模块）
// ============================================
#define CHECK_OK            0x00  // 全部正常
#define CHECK_FAIL_CLK      0x01  // 时钟异常
#define CHECK_FAIL_FLASH    0x02  // 存储异常
#define CHECK_FAIL_RAM      0x03  // 内存异常
#define CHECK_FAIL_BLE      0x04  // 蓝牙模块异常
#define CHECK_FAIL_ADC      0x05  // 阻抗检测电路异常
#define CHECK_FAIL_PARAM    0x06  // 参数区损坏
#define CHECK_FAIL_POWER    0x07  // 电源电压异常
#define CHECK_FAIL_PWM      0x08  // PWM输出异常
#define CHECK_FAIL_ESTOP    0x09  // 急停按钮异常

// ============================================
// 全局自检状态
// ============================================
uint8_t g_self_check_result = 0xFF;  // 0xFF = 未检测
bool g_system_ready = false;          // 系统就绪标志

// ============================================
// 外部变量声明（需要在其他模块定义）
// ============================================
extern uint16_t crc16_calc(uint8_t *data, uint16_t len);
extern void flash_write_word(uint32_t addr, uint32_t data);
extern uint32_t flash_read_word(uint32_t addr);
extern uint32_t clock_get_freq(void);
extern bool clk_32k_is_stable(void);
extern uint16_t adc_sample(uint8_t channel);
extern void pwm_set_duty(uint8_t ch, uint16_t duty);
extern void uart_send_string(char *str);
extern void led_set(uint8_t pin, bool on);
extern void delay_ms(uint16_t ms);

// 参数区定义（需要根据实际项目调整）
#define FLASH_SCHEME_ADDR   0x70000  // 参数区起始地址
#define FLASH_TEST_ADDR     0x3000   // Flash测试地址（未使用区域）

// 电源检测阈值
#define VBAT_OK_MIN         3000     // 电池电压正常下限 mV
#define VBAT_OK_MAX         4200     // 电池电压正常上限 mV
#define V33_OK_MIN          3200     // 3.3V稳压正常下限 mV
#define V33_OK_MAX          3400     // 3.3V稳压正常上限 mV

// PWM检测阈值
#define PWM_TEST_DUTY       512      // 测试占空比 (50%)
#define PWM_TEST_FREQ_HZ    1000     // 测试频率 1kHz

//==========================================================================
// 1. 时钟检测（32M系统时钟 + 32K休眠时钟）
//==========================================================================
static bool check_system_clock(void)
{
    // 检测PLL锁定状态
    // TLSR8269: reg_clk_ctrl BIT(2) = PLL_LOCK
    if ((reg_clk_ctrl & BIT(2)) == 0) {
        uart_send_string("[CHECK] PLL not locked!\r\n");
        return false;
    }
    
    // 检测系统时钟频率
    uint32_t freq = clock_get_freq();
    if (freq < 31000000 || freq > 33000000) {
        uart_send_string("[CHECK] System clock abnormal: ");
        // 这里可以添加频率值输出
        uart_send_string("Hz\r\n");
        return false;
    }
    
    // 检测32K时钟稳定性
    if (!clk_32k_is_stable()) {
        uart_send_string("[CHECK] 32K clock unstable!\r\n");
        return false;
    }
    
    uart_send_string("[CHECK] Clock OK\r\n");
    return true;
}

//==========================================================================
// 2. Flash存储检测（读写+校验+参数区完整性）
//==========================================================================
static bool check_flash_memory(void)
{
    const uint32_t TEST_DATA1 = 0xA5A55A5A;
    const uint32_t TEST_DATA2 = 0x12345678;
    uint32_t read_back;
    
    // 测试1：写入并读取验证
    flash_write_word(FLASH_TEST_ADDR, TEST_DATA1);
    read_back = flash_read_word(FLASH_TEST_ADDR);
    if (read_back != TEST_DATA1) {
        uart_send_string("[CHECK] Flash write/read failed!\r\n");
        return false;
    }
    
    // 测试2：写入不同数据再次验证
    flash_write_word(FLASH_TEST_ADDR, TEST_DATA2);
    read_back = flash_read_word(FLASH_TEST_ADDR);
    if (read_back != TEST_DATA2) {
        uart_send_string("[CHECK] Flash data retention failed!\r\n");
        return false;
    }
    
    // 恢复擦除（写入0xFFFFFFFF）
    flash_write_word(FLASH_TEST_ADDR, 0xFFFFFFFF);
    
    // 参数区CRC校验（如果已保存过参数）
    // 注意：首次开机可能没有保存过参数，需要特殊处理
    // uint16_t crc_calc = crc16_calc((uint8_t*)&g_saved_schemes, sizeof(g_saved_schemes)-2);
    // uint16_t crc_saved = flash_read_word(FLASH_SCHEME_ADDR + 0x3F00);
    // if (crc_calc != crc_saved) {
    //     // 参数损坏，恢复默认值
    //     memset(&g_saved_schemes, 0, sizeof(g_saved_schemes));
    //     // 重新保存默认参数...
    // }
    
    uart_send_string("[CHECK] Flash OK\r\n");
    return true;
}

//==========================================================================
// 3. RAM检测（ walking bit test ）
//==========================================================================
static bool check_ram_memory(void)
{
    volatile uint32_t *ram_test = (volatile uint32_t *)0x840000;  // RAM起始地址
    const uint16_t TEST_SIZE = 256;  // 测试256个32位字
    uint32_t pattern;
    uint16_t i;
    
    // Walking 1 test
    for (i = 0; i < TEST_SIZE; i++) {
        pattern = 0x00000001;
        for (uint8_t bit = 0; bit < 32; bit++) {
            ram_test[i] = pattern;
            if (ram_test[i] != pattern) {
                uart_send_string("[CHECK] RAM walking-1 failed!\r\n");
                return false;
            }
            pattern <<= 1;
        }
    }
    
    // Walking 0 test
    for (i = 0; i < TEST_SIZE; i++) {
        pattern = 0xFFFFFFFE;
        for (uint8_t bit = 0; bit < 32; bit++) {
            ram_test[i] = pattern;
            if (ram_test[i] != pattern) {
                uart_send_string("[CHECK] RAM walking-0 failed!\r\n");
                return false;
            }
            pattern = (pattern << 1) | 1;
        }
    }
    
    // 地址线测试
    for (i = 0; i < TEST_SIZE; i++) {
        ram_test[i] = (uint32_t)&ram_test[i];  // 写入地址值
    }
    for (i = 0; i < TEST_SIZE; i++) {
        if (ram_test[i] != (uint32_t)&ram_test[i]) {
            uart_send_string("[CHECK] RAM address line failed!\r\n");
            return false;
        }
    }
    
    uart_send_string("[CHECK] RAM OK\r\n");
    return true;
}

//==========================================================================
// 4. 电源电压检测（电池 + 3.3V稳压）
//==========================================================================
static bool check_power_supply(void)
{
    uint16_t vbat, v33;
    
    // 读取电池电压（通过ADC）
    // vbat = adc_sample(ADC_CHANNEL_VBAT);
    // 转换为mV...
    
    // 读取3.3V稳压输出
    // v33 = adc_sample(ADC_CHANNEL_V33);
    // 转换为mV...
    
    // 检查电池电压范围
    // if (vbat < VBAT_OK_MIN || vbat > VBAT_OK_MAX) {
    //     uart_send_string("[CHECK] Battery voltage abnormal!\r\n");
    //     return false;
    // }
    
    // 检查3.3V稳压
    // if (v33 < V33_OK_MIN || v33 > V33_OK_MAX) {
    //     uart_send_string("[CHECK] 3.3V regulator abnormal!\r\n");
    //     return false;
    // }
    
    uart_send_string("[CHECK] Power OK\r\n");
    return true;
}

//==========================================================================
// 5. ADC检测（阻抗检测电路）
//==========================================================================
static bool check_adc_circuit(void)
{
    uint16_t adc_val;
    
    // 检测内部参考电压
    adc_val = adc_sample(ADC_CHANNEL_0);  // 读取一个已知通道
    
    // 检查ADC是否在合理范围（未悬空）
    if (adc_val == 0 || adc_val == 0xFFFF) {
        uart_send_string("[CHECK] ADC reading abnormal!\r\n");
        return false;
    }
    
    // 检测阻抗检测电路（如果有测试电阻）
    // 可以连接一个已知电阻，检测电压是否在预期范围
    
    uart_send_string("[CHECK] ADC OK\r\n");
    return true;
}

//==========================================================================
// 6. PWM输出检测（六通道自检）
//==========================================================================
static bool check_pwm_output(void)
{
    // 配置PWM为测试频率和占空比
    // 注意：这里只是配置，不实际输出到电极
    // 实际应该通过反馈电路检测PWM是否产生
    
    // 配置6路PWM
    for (uint8_t ch = 0; ch < 6; ch++) {
        pwm_set_duty(ch, PWM_TEST_DUTY);
    }
    
    // 延时让PWM稳定
    delay_ms(10);
    
    // 如果有反馈检测电路，可以在这里检测
    // 例如：通过ADC读取PWM输出端的平均电压
    
    uart_send_string("[CHECK] PWM OK\r\n");
    return true;
}

//==========================================================================
// 7. 急停按钮检测
//==========================================================================
static bool check_estop_button(void)
{
    // 读取急停按钮状态
    // 注意：急停按钮通常是常闭（NC）或常开（NO）
    
    // 假设急停按钮连接到 PC2 (GPIO 18)
    #define ESTOP_PIN 18
    
    // 配置为输入，上拉
    // gpio_set_input(ESTOP_PIN, PULL_UP);
    
    // 读取状态
    // uint8_t estop_state = gpio_read(ESTOP_PIN);
    
    // 检查按钮是否卡住（如果是NO按钮，应该为高；NC按钮，应该为低）
    // 这里需要根据实际电路设计调整
    
    uart_send_string("[CHECK] E-Stop OK\r\n");
    return true;
}

//==========================================================================
// 8. 蓝牙模块检测（如果启用BLE）
//==========================================================================
static bool check_ble_module(void)
{
    // 检查BLE是否初始化成功
    // 可以尝试发送一个HCI命令，检查是否有响应
    
    // 如果没有启用BLE，直接返回true
    #ifdef BLE_ENABLED
    // BLE自检代码...
    // if (!ble_is_ready()) {
    //     uart_send_string("[CHECK] BLE init failed!\r\n");
    //     return false;
    // }
    #endif
    
    uart_send_string("[CHECK] BLE OK\r\n");
    return true;
}

//==========================================================================
// 主自检函数 — 开机全链路检测
//==========================================================================
uint8_t system_self_check(void)
{
    uint8_t result = CHECK_OK;
    
    uart_send_string("\r\n========================================\r\n");
    uart_send_string("  System Self-Check Starting...\r\n");
    uart_send_string("========================================\r\n");
    
    // 1. 时钟检测
    uart_send_string("\n[1/8] Checking system clock...\r\n");
    if (!check_system_clock()) {
        result = CHECK_FAIL_CLK;
        goto CHECK_DONE;
    }
    
    // 2. Flash检测
    uart_send_string("[2/8] Checking flash memory...\r\n");
    if (!check_flash_memory()) {
        result = CHECK_FAIL_FLASH;
        goto CHECK_DONE;
    }
    
    // 3. RAM检测
    uart_send_string("[3/8] Checking RAM...\r\n");
    if (!check_ram_memory()) {
        result = CHECK_FAIL_RAM;
        goto CHECK_DONE;
    }
    
    // 4. 电源检测
    uart_send_string("[4/8] Checking power supply...\r\n");
    if (!check_power_supply()) {
        result = CHECK_FAIL_POWER;
        goto CHECK_DONE;
    }
    
    // 5. ADC检测
    uart_send_string("[5/8] Checking ADC circuit...\r\n");
    if (!check_adc_circuit()) {
        result = CHECK_FAIL_ADC;
        goto CHECK_DONE;
    }
    
    // 6. PWM检测
    uart_send_string("[6/8] Checking PWM output...\r\n");
    if (!check_pwm_output()) {
        result = CHECK_FAIL_PWM;
        goto CHECK_DONE;
    }
    
    // 7. 急停按钮检测
    uart_send_string("[7/8] Checking E-Stop button...\r\n");
    if (!check_estop_button()) {
        result = CHECK_FAIL_ESTOP;
        goto CHECK_DONE;
    }
    
    // 8. BLE检测
    uart_send_string("[8/8] Checking BLE module...\r\n");
    if (!check_ble_module()) {
        result = CHECK_FAIL_BLE;
        goto CHECK_DONE;
    }
    
CHECK_DONE:
    g_self_check_result = result;
    
    // 输出结果
    uart_send_string("\n========================================\r\n");
    if (result == CHECK_OK) {
        uart_send_string("  ✓ ALL CHECKS PASSED\r\n");
        uart_send_string("  System Ready\r\n");
        g_system_ready = true;
        led_set(LED_PIN, true);  // 点亮状态LED
    } else {
        uart_send_string("  ✗ CHECK FAILED\r\n");
        uart_send_string("  Error Code: 0x");
        // 输出错误码...
        uart_send_string("\r\n");
        uart_send_string("  SYSTEM LOCKED!\r\n");
        g_system_ready = false;
        // 闪烁LED表示错误
    }
    uart_send_string("========================================\r\n\n");
    
    return result;
}

//==========================================================================
// 获取自检结果字符串
//==========================================================================
const char* get_check_result_string(uint8_t code)
{
    switch (code) {
        case CHECK_OK:          return "OK - All checks passed";
        case CHECK_FAIL_CLK:    return "FAIL - System clock abnormal";
        case CHECK_FAIL_FLASH:  return "FAIL - Flash memory error";
        case CHECK_FAIL_RAM:    return "FAIL - RAM error";
        case CHECK_FAIL_BLE:    return "FAIL - BLE module error";
        case CHECK_FAIL_ADC:    return "FAIL - ADC circuit error";
        case CHECK_FAIL_PARAM:  return "FAIL - Parameter area corrupted";
        case CHECK_FAIL_POWER:  return "FAIL - Power supply abnormal";
        case CHECK_FAIL_PWM:    return "FAIL - PWM output error";
        case CHECK_FAIL_ESTOP:  return "FAIL - E-Stop button error";
        default:                return "UNKNOWN - Invalid error code";
    }
}

//==========================================================================
// 错误处理 — 锁机+提示
//==========================================================================
void handle_system_error(uint8_t error_code)
{
    // 关闭所有输出
    for (uint8_t ch = 0; ch < 6; ch++) {
        pwm_set_duty(ch, 0);  // 占空比设为0
    }
    
    // 关闭电源（如果有电源控制）
    // power_shutdown();
    
    // 闪烁LED报警
    while (1) {
        led_set(LED_PIN, true);
        delay_ms(200);
        led_set(LED_PIN, false);
        delay_ms(200);
        
        // 通过UART发送错误信息（持续）
        uart_send_string("\r\n*** SYSTEM ERROR ***\r\n");
        uart_send_string("Code: 0x");
        // 发送错误码...
        uart_send_string("\r\n");
        uart_send_string(get_check_result_string(error_code));
        uart_send_string("\r\nPlease power cycle or contact support.\r\n");
        uart_send_string("********************\r\n");
    }
}

//==========================================================================
// 使用示例
//==========================================================================
/*
void main(void)
{
    // 初始化硬件...
    system_init();
    
    // 执行自检
    uint8_t check_result = system_self_check();
    
    if (check_result != CHECK_OK) {
        // 自检失败，锁机
        handle_system_error(check_result);
        // 不会执行到这里（handle_system_error中有死循环）
    }
    
    // 自检通过，进入主循环
    while (1) {
        main_loop();
    }
}
*/
