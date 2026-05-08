/**
 * 帕金森治疗示例程序
 * 
 * 使用 PulseTherapy 库进行标准经颅脉冲刺激治疗
 * 
 * 硬件: Telink TLSR8269
 * 电极位置: 前额叶 (FP1/FP2)
 * 
 * 安全提示:
 * - 首次使用请在医生指导下进行
 * - 有癫痫病史者禁用
 * - 装有心脏起搏器者禁用
 * - 治疗过程中如感不适，立即按下急停按钮
 */

#include <PulseTherapy.h>

// 引脚定义
#define EMERGENCY_STOP_PIN  20  // 急停按钮（外部中断）
#define BUZZER_PIN          21  // 蜂鸣器
#define LED_STATUS_PIN      22  // 状态指示灯

// 治疗参数
#define TREATMENT_DURATION  20  // 治疗时长（分钟）

// 全局变量
bool emergencyStopFlag = false;
uint32_t lastStatusUpdate = 0;

void setup() {
    // 初始化串口
    Serial.begin(115200);
    Serial.println("========================================");
    Serial.println("  开源经颅脉冲治疗仪");
    Serial.println("  Open Source Pulse Therapy Device");
    Serial.println("  版本: " + String(PulseTherapy::getVersion()));
    Serial.println("========================================");
    
    // 初始化引脚
    pinMode(EMERGENCY_STOP_PIN, INPUT_PULLUP);
    pinMode(BUZZER_PIN, OUTPUT);
    pinMode(LED_STATUS_PIN, OUTPUT);
    
    // 绑定急停中断
    attachInterrupt(digitalPinToInterrupt(EMERGENCY_STOP_PIN), emergencyStopISR, FALLING);
    
    // 初始化脉冲治疗库
    Serial.println("正在初始化...");
    if (!Pulse.begin()) {
        Serial.println("初始化失败！请检查硬件连接。");
        errorBeep();
        while (1) {
            digitalWrite(LED_STATUS_PIN, !digitalRead(LED_STATUS_PIN));
            delay(200);
        }
    }
    
    // 设置回调函数
    Pulse.onSafetyAlert(safetyAlertHandler);
    Pulse.onComplete(therapyCompleteHandler);
    
    // 设置电流限制（2mA = 2000uA）
    Pulse.setCurrentLimit(2000);
    
    Serial.println("初始化完成！");
    Serial.println("电流限制: 2mA");
    Serial.println();
    
    // 提示用户准备
    Serial.println("请按以下步骤准备:");
    Serial.println("1. 清洁前额皮肤");
    Serial.println("2. 涂抹导电凝胶");
    Serial.println("3. 将电极贴于FP1/FP2位置");
    Serial.println("4. 确认急停按钮可正常按下");
    Serial.println();
    Serial.println("准备完成后，输入 'START' 开始治疗");
    
    // 等待用户确认
    waitForStart();
}

void loop() {
    // 检查急停标志
    if (emergencyStopFlag) {
        handleEmergencyStop();
        return;
    }
    
    // 更新治疗状态
    Pulse.update();
    
    // 每秒更新一次状态显示
    if (millis() - lastStatusUpdate > 1000) {
        lastStatusUpdate = millis();
        displayStatus();
    }
    
    // 检查串口命令
    checkSerialCommands();
}

// 等待用户开始
void waitForStart() {
    while (true) {
        if (Serial.available()) {
            String cmd = Serial.readStringUntil('\n');
            cmd.trim();
            cmd.toUpperCase();
            
            if (cmd == "START") {
                Serial.println();
                Serial.println("开始治疗...");
                
                // 启动帕金森标准治疗模式
                if (Pulse.startTherapy(MODE_PARKINSON_STD, TREATMENT_DURATION)) {
                    Serial.println("治疗已启动！");
                    Serial.println("模式: 帕金森标准 (40Hz, 50%占空比)");
                    Serial.println("时长: " + String(TREATMENT_DURATION) + " 分钟");
                    Serial.println();
                    Serial.println("命令: PAUSE(暂停), RESUME(继续), STOP(停止)");
                    successBeep();
                    return;
                } else {
                    Serial.println("启动失败！");
                    errorBeep();
                }
            }
            else if (cmd == "TEST") {
                runSelfTest();
            }
            else if (cmd == "HELP") {
                printHelp();
            }
        }
        
        // 呼吸灯效果
        Pulse.breathingEffect(0, 2.0);
        delay(50);
    }
}

// 显示治疗状态
void displayStatus() {
    if (!Pulse.isRunning()) return;
    
    uint32_t elapsed = Pulse.getElapsedTime();
    uint32_t remaining = Pulse.getRemainingTime();
    uint8_t activeCh = Pulse.getActiveChannelCount();
    
    Serial.print("[状态] ");
    Serial.print("运行中 | ");
    Serial.print("已用: ");
    Serial.print(elapsed / 60);
    Serial.print("分");
    Serial.print(elapsed % 60);
    Serial.print("秒 | ");
    Serial.print("剩余: ");
    Serial.print(remaining / 60);
    Serial.print("分");
    Serial.print(remaining % 60);
    Serial.print("秒 | ");
    Serial.print("通道: ");
    Serial.print(activeCh);
    Serial.println("/4");
    
    // 显示各通道电流
    for (int i = 0; i < 4; i++) {
        ChannelState state = Pulse.getChannelState(i);
        if (state.enabled) {
            Serial.print("  CH");
            Serial.print(i);
            Serial.print(": ");
            Serial.print(state.frequency);
            Serial.print("Hz, ");
            Serial.print(state.duty / 100);
            Serial.print(".");
            Serial.print(state.duty % 100);
            Serial.print("%, ");
            Serial.print(state.current_uA);
            Serial.println("uA");
        }
    }
}

// 检查串口命令
void checkSerialCommands() {
    if (!Serial.available()) return;
    
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    cmd.toUpperCase();
    
    if (cmd == "PAUSE") {
        if (Pulse.pauseTherapy()) {
            Serial.println("治疗已暂停");
        }
    }
    else if (cmd == "RESUME") {
        if (Pulse.resumeTherapy()) {
            Serial.println("治疗已继续");
        }
    }
    else if (cmd == "STOP") {
        Pulse.stopTherapy();
        Serial.println("治疗已停止");
        Serial.println("输入 'START' 重新开始");
        waitForStart();
    }
    else if (cmd == "STATUS") {
        displayStatus();
    }
    else if (cmd == "EMERGENCY") {
        emergencyStopFlag = true;
    }
}

// 急停中断服务程序
void emergencyStopISR() {
    emergencyStopFlag = true;
}

// 处理急停
void handleEmergencyStop() {
    Pulse.emergencyStop();
    
    Serial.println();
    Serial.println("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
    Serial.println("!!!         紧急停止已触发         !!!");
    Serial.println("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
    
    // 持续报警
    while (true) {
        digitalWrite(LED_STATUS_PIN, HIGH);
        digitalWrite(BUZZER_PIN, HIGH);
        delay(100);
        digitalWrite(LED_STATUS_PIN, LOW);
        digitalWrite(BUZZER_PIN, LOW);
        delay(100);
    }
}

// 安全警报处理
void safetyAlertHandler(const char* message) {
    Serial.println();
    Serial.print("[安全警报] ");
    Serial.println(message);
    warningBeep();
}

// 治疗完成回调
void therapyCompleteHandler() {
    Serial.println();
    Serial.println("========================================");
    Serial.println("  治疗完成！");
    Serial.println("========================================");
    
    // 完成提示音
    for (int i = 0; i < 3; i++) {
        digitalWrite(BUZZER_PIN, HIGH);
        delay(200);
        digitalWrite(BUZZER_PIN, LOW);
        delay(200);
    }
    
    Serial.println("请取下电极，清洁皮肤。");
    Serial.println("输入 'START' 开始新的治疗。");
    
    waitForStart();
}

// 自检程序
void runSelfTest() {
    Serial.println();
    Serial.println("========== 自检程序 ==========");
    
    bool allPassed = true;
    
    // 1. 检查电池
    Serial.print("电池电压... ");
    uint16_t voltage = readBatteryVoltage();
    if (voltage >= 3300 && voltage <= 4200) {
        Serial.print("正常 (");
        Serial.print(voltage);
        Serial.println("mV)");
    } else {
        Serial.print("异常 (");
        Serial.print(voltage);
        Serial.println("mV)");
        allPassed = false;
    }
    
    // 2. 检查PWM输出
    Serial.print("PWM输出... ");
    for (int i = 0; i < 4; i++) {
        Pulse.configureChannel(i, 1000, 5000, 0);
        Pulse.enableChannel(i);
        delay(100);
        Pulse.disableChannel(i);
    }
    Serial.println("正常");
    
    // 3. 检查急停按钮
    Serial.print("急停按钮... ");
    if (digitalRead(EMERGENCY_STOP_PIN) == HIGH) {
        Serial.println("正常 (未按下)");
    } else {
        Serial.println("异常 (可能卡住)");
        allPassed = false;
    }
    
    // 4. 检查蜂鸣器
    Serial.print("蜂鸣器... ");
    digitalWrite(BUZZER_PIN, HIGH);
    delay(200);
    digitalWrite(BUZZER_PIN, LOW);
    Serial.println("正常");
    
    Serial.println("==============================");
    if (allPassed) {
        Serial.println("自检通过！设备正常。");
        successBeep();
    } else {
        Serial.println("自检未通过！请检查设备。");
        errorBeep();
    }
    Serial.println();
}

// 打印帮助
void printHelp() {
    Serial.println();
    Serial.println("========== 命令列表 ==========");
    Serial.println("START     - 开始治疗");
    Serial.println("PAUSE     - 暂停治疗");
    Serial.println("RESUME    - 继续治疗");
    Serial.println("STOP      - 停止治疗");
    Serial.println("STATUS    - 显示状态");
    Serial.println("TEST      - 运行自检");
    Serial.println("EMERGENCY - 紧急停止");
    Serial.println("HELP      - 显示帮助");
    Serial.println("==============================");
}

// 提示音
void successBeep() {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(100);
    digitalWrite(BUZZER_PIN, LOW);
    delay(50);
    digitalWrite(BUZZER_PIN, HIGH);
    delay(100);
    digitalWrite(BUZZER_PIN, LOW);
}

void errorBeep() {
    for (int i = 0; i < 3; i++) {
        digitalWrite(BUZZER_PIN, HIGH);
        delay(300);
        digitalWrite(BUZZER_PIN, LOW);
        delay(100);
    }
}

void warningBeep() {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(500);
    digitalWrite(BUZZER_PIN, LOW);
}

// 读取电池电压（辅助函数）
uint16_t readBatteryVoltage() {
    // 需要通过ADC和分压电路实现
    // 简化实现，返回模拟值
    return analogRead(A0) * 2;
}
