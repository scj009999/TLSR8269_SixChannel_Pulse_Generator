/**
 * Mixly 代码生成器 - TLSR8269 六路脉冲发生器
 * 
 * 将积木块转换为C代码
 */

'use strict';

goog.provide('Blockly.Arduino.pulse_generator');
goog.require('Blockly.Arduino');

// ==================== 脉冲输出代码生成 ====================

Blockly.Arduino['pulse_init'] = function(block) {
    Blockly.Arduino.definitions_['include_pulse'] = '#include "pulse_generator.h"';
    Blockly.Arduino.setups_['pulse_init'] = 'pulse_generator_init();';
    return '';
};

Blockly.Arduino['pulse_channel_config'] = function(block) {
    var channel = block.getFieldValue('CHANNEL');
    var freq = Blockly.Arduino.valueToCode(block, 'FREQ', Blockly.Arduino.ORDER_ATOMIC) || '1000';
    var duty = Blockly.Arduino.valueToCode(block, 'DUTY', Blockly.Arduino.ORDER_ATOMIC) || '50';
    
    // 将百分比转换为0-10000
    var duty_scaled = '(' + duty + ' * 100)';
    
    var code = 'pulse_channel_config(' + channel + ', ' + freq + ', ' + duty_scaled + ', 0);\n';
    return code;
};

Blockly.Arduino['pulse_channel_enable'] = function(block) {
    var action = block.getFieldValue('ACTION');
    var channel = block.getFieldValue('CHANNEL');
    var code = '';
    
    if (channel === 'ALL') {
        if (action === 'ENABLE') {
            code = 'for (int i = 0; i < 6; i++) { pulse_channel_enable(i); }\n';
        } else {
            code = 'for (int i = 0; i < 6; i++) { pulse_channel_disable(i); }\n';
        }
    } else {
        if (action === 'ENABLE') {
            code = 'pulse_channel_enable(' + channel + ');\n';
        } else {
            code = 'pulse_channel_disable(' + channel + ');\n';
        }
    }
    return code;
};

Blockly.Arduino['pulse_set_frequency'] = function(block) {
    var channel = block.getFieldValue('CHANNEL');
    var freq = Blockly.Arduino.valueToCode(block, 'FREQ', Blockly.Arduino.ORDER_ATOMIC) || '1000';
    var code = 'pulse_set_frequency(' + channel + ', ' + freq + ');\n';
    return code;
};

Blockly.Arduino['pulse_set_duty'] = function(block) {
    var channel = block.getFieldValue('CHANNEL');
    var duty = Blockly.Arduino.valueToCode(block, 'DUTY', Blockly.Arduino.ORDER_ATOMIC) || '50';
    var duty_scaled = '(' + duty + ' * 100)';
    var code = 'pulse_set_duty(' + channel + ', ' + duty_scaled + ');\n';
    return code;
};

// ==================== 治疗模式代码生成 ====================

Blockly.Arduino['pulse_treatment_mode'] = function(block) {
    var mode = block.getFieldValue('MODE');
    var duration = Blockly.Arduino.valueToCode(block, 'DURATION', Blockly.Arduino.ORDER_ATOMIC) || '20';
    
    var code = '';
    
    switch(mode) {
        case 'PARKINSON_STD':
            code += '// 帕金森标准治疗模式\n';
            code += '// 基于刘河生教授研究参数\n';
            code += 'pulse_channel_config(0, 40, 5000, 0);  // 40Hz, 50%占空比\n';
            code += 'pulse_channel_config(1, 40, 5000, 0);\n';
            code += 'pulse_channel_config(2, 40, 5000, 0);\n';
            code += 'pulse_channel_config(3, 40, 5000, 0);\n';
            code += 'pulse_channel_enable(0);\n';
            code += 'pulse_channel_enable(1);\n';
            code += 'pulse_channel_enable(2);\n';
            code += 'pulse_channel_enable(3);\n';
            break;
        case 'LOW_FREQ':
            code += '// 低频舒缓模式\n';
            code += 'pulse_channel_config(0, 10, 3000, 0);  // 10Hz, 30%占空比\n';
            code += 'pulse_channel_config(1, 10, 3000, 0);\n';
            code += 'pulse_channel_enable(0);\n';
            code += 'pulse_channel_enable(1);\n';
            break;
        case 'HIGH_FREQ':
            code += '// 高频激活模式\n';
            code += 'pulse_channel_config(0, 100, 7000, 0);  // 100Hz, 70%占空比\n';
            code += 'pulse_channel_config(1, 100, 7000, 0);\n';
            code += 'pulse_channel_enable(0);\n';
            code += 'pulse_channel_enable(1);\n';
            break;
        case 'CUSTOM':
            code += '// 自定义模式 - 请在下方配置参数\n';
            break;
    }
    
    code += '// 治疗时长: ' + duration + ' 分钟\n';
    code += 'treatment_duration_minutes = ' + duration + ';\n';
    
    return code;
};

Blockly.Arduino['pulse_parkinson_params'] = function(block) {
    var scheme = block.getFieldValue('SCHEME');
    var code = '';
    
    switch(scheme) {
        case 'LIU_STD':
            code = '"频率:40Hz, 脉宽:200us, 电流:2mA, 时长:20min"';
            break;
        case 'LIU_INTENSE':
            code = '"频率:40Hz, 脉宽:500us, 电流:4mA, 时长:30min"';
            break;
        case 'CONSERVATIVE':
            code = '"频率:20Hz, 脉宽:100us, 电流:1mA, 时长:15min"';
            break;
    }
    
    return [code, Blockly.Arduino.ORDER_ATOMIC];
};

// ==================== 安全控制代码生成 ====================

Blockly.Arduino['pulse_emergency_stop'] = function(block) {
    var code = '';
    code += '// 紧急停止 - 立即切断所有输出\n';
    code += 'for (int i = 0; i < 6; i++) {\n';
    code += '  pulse_channel_disable(i);\n';
    code += '}\n';
    code += 'digitalWrite(EMERGENCY_STOP_PIN, HIGH);\n';
    code += 'emergency_triggered = true;\n';
    code += 'display_show_error("EMERGENCY STOP");\n';
    return code;
};

Blockly.Arduino['pulse_safety_check'] = function(block) {
    var code = 'safety_check_passed()';
    return [code, Blockly.Arduino.ORDER_FUNCTION_CALL];
};

Blockly.Arduino['pulse_current_limit'] = function(block) {
    var limit = block.getFieldValue('LIMIT');
    var code = 'set_current_limit(' + (limit * 1000) + ');  // 转换为uA\n';
    return code;
};

// ==================== 高级功能代码生成 ====================

Blockly.Arduino['pulse_phase_sync'] = function(block) {
    var master = block.getFieldValue('MASTER');
    var code = '// 以通道' + master + '为基准同步相位\n';
    code += 'pulse_phase_sync(' + master + ');\n';
    return code;
};

Blockly.Arduino['pulse_breathing_effect'] = function(block) {
    var channel = block.getFieldValue('CHANNEL');
    var period = block.getFieldValue('PERIOD');
    
    var code = '';
    code += '// 呼吸灯效果\n';
    code += '{\n';
    code += '  static unsigned long lastBreathTime = 0;\n';
    code += '  static int breathDirection = 1;\n';
    code += '  static int breathDuty = 0;\n';
    code += '  unsigned long now = millis();\n';
    code += '  if (now - lastBreathTime > ' + (period * 1000 / 200) + ') {\n';
    code += '    lastBreathTime = now;\n';
    code += '    breathDuty += breathDirection * 100;\n';
    code += '    if (breathDuty >= 10000) { breathDuty = 10000; breathDirection = -1; }\n';
    code += '    if (breathDuty <= 0) { breathDuty = 0; breathDirection = 1; }\n';
    code += '    pulse_set_duty(' + channel + ', breathDuty);\n';
    code += '  }\n';
    code += '}\n';
    
    return code;
};

// ==================== 传感器代码生成 ====================

Blockly.Arduino['pulse_read_battery'] = function(block) {
    var code = 'read_battery_voltage()';
    return [code, Blockly.Arduino.ORDER_FUNCTION_CALL];
};

Blockly.Arduino['pulse_read_temperature'] = function(block) {
    var code = 'read_temperature()';
    return [code, Blockly.Arduino.ORDER_FUNCTION_CALL];
};

// ==================== 显示代码生成 ====================

Blockly.Arduino['pulse_display_status'] = function(block) {
    var content = block.getFieldValue('CONTENT');
    var code = '';
    
    switch(content) {
        case 'CHANNEL_STATUS':
            code = 'display_channel_status();\n';
            break;
        case 'TREATMENT_PROGRESS':
            code = 'display_treatment_progress();\n';
            break;
        case 'BATTERY_LEVEL':
            code = 'display_battery_level();\n';
            break;
        case 'CUSTOM_TEXT':
            code = 'display_custom_text("Hello");\n';
            break;
    }
    
    return code;
};

// ==================== 完整示例代码生成 ====================

Blockly.Arduino['pulse_demo_parkinson'] = function(block) {
    var setup_code = Blockly.Arduino.statementToCode(block, 'SETUP');
    var loop_code = Blockly.Arduino.statementToCode(block, 'LOOP');
    
    var code = '';
    code += '// ==========================================\n';
    code += '// 帕金森治疗仪 - 开源硬件项目\n';
    code += '// 基于 TLSR8269 六路脉冲发生器\n';
    code += '// 项目地址: https://github.com/yourrepo\n';
    code += '// ==========================================\n\n';
    code += '#include "pulse_generator.h"\n';
    code += '#include "display.h"\n';
    code += '#include "safety.h"\n\n';
    code += '// 全局变量\n';
    code += 'int treatment_duration_minutes = 20;\n';
    code += 'bool emergency_triggered = false;\n\n';
    code += 'void setup() {\n';
    code += '  // 初始化串口\n';
    code += '  Serial.begin(115200);\n';
    code += '  \n';
    code += '  // 初始化脉冲发生器\n';
    code += '  pulse_generator_init();\n';
    code += '  \n';
    code += '  // 初始化显示屏\n';
    code += '  display_init();\n';
    code += '  \n';
    code += '  // 初始化安全系统\n';
    code += '  safety_init();\n';
    code += '  \n';
    code += setup_code;
    code += '}\n\n';
    code += 'void loop() {\n';
    code += '  // 安全检查\n';
    code += '  if (!safety_check_passed()) {\n';
    code += '    emergency_stop_all();\n';
    code += '    return;\n';
    code += '  }\n';
    code += '  \n';
    code += '  // 检查急停按钮\n';
    code += '  if (digitalRead(EMERGENCY_STOP_PIN) == LOW) {\n';
    code += '    emergency_stop_all();\n';
    code += '    return;\n';
    code += '  }\n';
    code += '  \n';
    code += loop_code;
    code += '}\n';
    
    return code;
};
