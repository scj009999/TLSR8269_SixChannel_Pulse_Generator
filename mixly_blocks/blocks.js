/**
 * Mixly 积木块定义 - TLSR8269 六路脉冲发生器
 * 
 * 为帕金森治疗仪设计的图形化编程积木
 */

'use strict';

goog.provide('Blockly.Blocks.pulse_generator');
goog.require('Blockly.Blocks');

// 颜色定义
var PULSE_COLOR = '#E74C3C';      // 红色系 - 脉冲相关
var CHANNEL_COLOR = '#3498DB';    // 蓝色系 - 通道控制
var SAFETY_COLOR = '#2ECC71';     // 绿色系 - 安全功能

// ==================== 脉冲输出积木 ====================

Blockly.Blocks['pulse_init'] = {
    init: function() {
        this.setColour(PULSE_COLOR);
        this.appendDummyInput()
            .appendField("初始化脉冲发生器")
            .appendField(new Blockly.FieldImage("/static/pulse_icon.png", 20, 20));
        this.setPreviousStatement(true);
        this.setNextStatement(true);
        this.setTooltip("初始化TLSR8269六路脉冲发生器，配置PWM时钟和GPIO");
        this.setHelpUrl("https://github.com/yourrepo/pulse_generator");
    }
};

Blockly.Blocks['pulse_channel_config'] = {
    init: function() {
        this.setColour(PULSE_COLOR);
        this.appendDummyInput()
            .appendField("配置脉冲通道")
            .appendField(new Blockly.FieldDropdown([
                ["通道0 (PA0)", "0"],
                ["通道1 (PA1)", "1"],
                ["通道2 (PA2)", "2"],
                ["通道3 (PA3)", "3"],
                ["通道4 (PA4)", "4"],
                ["通道5 (PA5)", "5"]
            ]), "CHANNEL");
        this.appendValueInput("FREQ")
            .setCheck("Number")
            .appendField("频率(Hz)");
        this.appendValueInput("DUTY")
            .setCheck("Number")
            .appendField("占空比(%)");
        this.setPreviousStatement(true);
        this.setNextStatement(true);
        this.setTooltip("配置指定通道的脉冲频率和占空比");
        this.setHelpUrl("");
    }
};

Blockly.Blocks['pulse_channel_enable'] = {
    init: function() {
        this.setColour(CHANNEL_COLOR);
        this.appendDummyInput()
            .appendField(new Blockly.FieldDropdown([
                ["启动", "ENABLE"],
                ["停止", "DISABLE"]
            ]), "ACTION")
            .appendField("脉冲通道")
            .appendField(new Blockly.FieldDropdown([
                ["通道0", "0"],
                ["通道1", "1"],
                ["通道2", "2"],
                ["通道3", "3"],
                ["通道4", "4"],
                ["通道5", "5"],
                ["全部通道", "ALL"]
            ]), "CHANNEL");
        this.setPreviousStatement(true);
        this.setNextStatement(true);
        this.setTooltip("启动或停止指定脉冲通道的输出");
    }
};

Blockly.Blocks['pulse_set_frequency'] = {
    init: function() {
        this.setColour(PULSE_COLOR);
        this.appendDummyInput()
            .appendField("设置通道")
            .appendField(new Blockly.FieldDropdown([
                ["通道0", "0"],
                ["通道1", "1"],
                ["通道2", "2"],
                ["通道3", "3"],
                ["通道4", "4"],
                ["通道5", "5"]
            ]), "CHANNEL")
            .appendField("频率为");
        this.appendValueInput("FREQ")
            .setCheck("Number");
        this.appendDummyInput()
            .appendField("Hz");
        this.setPreviousStatement(true);
        this.setNextStatement(true);
        this.setTooltip("动态修改通道频率，范围1Hz-1MHz");
    }
};

Blockly.Blocks['pulse_set_duty'] = {
    init: function() {
        this.setColour(PULSE_COLOR);
        this.appendDummyInput()
            .appendField("设置通道")
            .appendField(new Blockly.FieldDropdown([
                ["通道0", "0"],
                ["通道1", "1"],
                ["通道2", "2"],
                ["通道3", "3"],
                ["通道4", "4"],
                ["通道5", "5"]
            ]), "CHANNEL")
            .appendField("占空比为");
        this.appendValueInput("DUTY")
            .setCheck("Number");
        this.appendDummyInput()
            .appendField("%");
        this.setPreviousStatement(true);
        this.setNextStatement(true);
        this.setTooltip("动态修改通道占空比，范围0%-100%");
    }
};

// ==================== 治疗模式积木 ====================

Blockly.Blocks['pulse_treatment_mode'] = {
    init: function() {
        this.setColour(SAFETY_COLOR);
        this.appendDummyInput()
            .appendField("开始治疗模式")
            .appendField(new Blockly.FieldDropdown([
                ["帕金森标准模式", "PARKINSON_STD"],
                ["低频舒缓模式", "LOW_FREQ"],
                ["高频激活模式", "HIGH_FREQ"],
                ["自定义模式", "CUSTOM"]
            ]), "MODE");
        this.appendValueInput("DURATION")
            .setCheck("Number")
            .appendField("治疗时长(分钟)");
        this.setPreviousStatement(true);
        this.setNextStatement(true);
        this.setTooltip("选择预设治疗模式并设置治疗时长");
    }
};

Blockly.Blocks['pulse_parkinson_params'] = {
    init: function() {
        this.setColour(SAFETY_COLOR);
        this.appendDummyInput()
            .appendField("帕金森治疗参数")
            .appendField(new Blockly.FieldDropdown([
                ["刘河生方案-标准", "LIU_STD"],
                ["刘河生方案-强化", "LIU_INTENSE"],
                ["保守方案", "CONSERVATIVE"]
            ]), "SCHEME");
        this.setOutput(true, "String");
        this.setTooltip("选择基于临床研究的治疗参数方案");
    }
};

// ==================== 安全控制积木 ====================

Blockly.Blocks['pulse_emergency_stop'] = {
    init: function() {
        this.setColour('#C0392B');
        this.appendDummyInput()
            .appendField("🛑 紧急停止")
            .appendField(new Blockly.FieldImage("/static/emergency_icon.png", 25, 25));
        this.setPreviousStatement(true);
        this.setNextStatement(false);
        this.setTooltip("立即停止所有脉冲输出，用于紧急情况");
    }
};

Blockly.Blocks['pulse_safety_check'] = {
    init: function() {
        this.setColour(SAFETY_COLOR);
        this.appendDummyInput()
            .appendField("安全检查通过?");
        this.setOutput(true, "Boolean");
        this.setTooltip("检查电极连接、电流、温度等安全参数");
    }
};

Blockly.Blocks['pulse_current_limit'] = {
    init: function() {
        this.setColour(SAFETY_COLOR);
        this.appendDummyInput()
            .appendField("设置输出电流上限")
            .appendField(new Blockly.FieldNumber(2, 0.1, 5, 0.1), "LIMIT")
            .appendField("mA");
        this.setPreviousStatement(true);
        this.setNextStatement(true);
        this.setTooltip("限制最大输出电流，保护患者安全（最大5mA）");
    }
};

// ==================== 高级功能积木 ====================

Blockly.Blocks['pulse_phase_sync'] = {
    init: function() {
        this.setColour(CHANNEL_COLOR);
        this.appendDummyInput()
            .appendField("同步通道相位")
            .appendField(new Blockly.FieldDropdown([
                ["通道0", "0"],
                ["通道1", "1"],
                ["通道2", "2"],
                ["通道3", "3"],
                ["通道4", "4"],
                ["通道5", "5"]
            ]), "MASTER")
            .appendField("为主通道");
        this.setPreviousStatement(true);
        this.setNextStatement(true);
        this.setTooltip("以指定通道为基准，同步其他通道相位");
    }
};

Blockly.Blocks['pulse_breathing_effect'] = {
    init: function() {
        this.setColour(PULSE_COLOR);
        this.appendDummyInput()
            .appendField("呼吸灯效果 通道")
            .appendField(new Blockly.FieldDropdown([
                ["通道0", "0"],
                ["通道1", "1"],
                ["通道2", "2"],
                ["通道3", "3"],
                ["通道4", "4"],
                ["通道5", "5"]
            ]), "CHANNEL")
            .appendField("周期")
            .appendField(new Blockly.FieldNumber(2, 0.5, 10, 0.5), "PERIOD")
            .appendField("秒");
        this.setPreviousStatement(true);
        this.setNextStatement(true);
        this.setTooltip("产生占空比渐变的呼吸效果，用于指示或舒缓");
    }
};

// ==================== 传感器积木 ====================

Blockly.Blocks['pulse_read_battery'] = {
    init: function() {
        this.setColour('#F39C12');
        this.appendDummyInput()
            .appendField("读取电池电压");
        this.setOutput(true, "Number");
        this.setTooltip("读取当前电池电压，低电量时提醒充电");
    }
};

Blockly.Blocks['pulse_read_temperature'] = {
    init: function() {
        this.setColour('#F39C12');
        this.appendDummyInput()
            .appendField("读取设备温度");
        this.setOutput(true, "Number");
        this.setTooltip("读取设备内部温度，过热时自动保护");
    }
};

// ==================== 显示积木 ====================

Blockly.Blocks['pulse_display_status'] = {
    init: function() {
        this.setColour('#9B59B6');
        this.appendDummyInput()
            .appendField("在OLED显示")
            .appendField(new Blockly.FieldDropdown([
                ["通道状态", "CHANNEL_STATUS"],
                ["治疗进度", "TREATMENT_PROGRESS"],
                ["电池电量", "BATTERY_LEVEL"],
                ["自定义文字", "CUSTOM_TEXT"]
            ]), "CONTENT");
        this.setPreviousStatement(true);
        this.setNextStatement(true);
        this.setTooltip("在0.96寸OLED显示屏上显示信息");
    }
};

// ==================== 完整示例积木 ====================

Blockly.Blocks['pulse_demo_parkinson'] = {
    init: function() {
        this.setColour(SAFETY_COLOR);
        this.appendDummyInput()
            .appendField("🧠 帕金森治疗示例程序");
        this.appendStatementInput("SETUP")
            .setCheck(null)
            .appendField("初始化");
        this.appendStatementInput("LOOP")
            .setCheck(null)
            .appendField("主循环");
        this.setTooltip("完整的帕金森治疗程序模板");
    }
};
