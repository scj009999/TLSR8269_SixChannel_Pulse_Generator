#!/bin/bash
# 修复构建问题的脚本

echo "=== 修复TLSR8269固件构建问题 ==="

# 1. 检查SDK路径
if [ -z "$TELINK_SDK_PATH" ]; then
    export TELINK_SDK_PATH=/root/telink_826x_sdk
fi

# 2. 检查工具链
if [ ! -f /opt/tc32/bin/tc32-elf-gcc ]; then
    echo "错误: TC32工具链未安装"
    exit 1
fi

export PATH=/opt/tc32/bin:$PATH

# 3. 创建必要的头文件修复
cat > irq_wrapper.h << 'EOF'
/**
 * @file irq_wrapper.h
 * @brief 中断控制包装器
 */
#ifndef __IRQ_WRAPPER_H__
#define __IRQ_WRAPPER_H__

#include "tlsr8269_reg.h"

static inline void irq_disable(void) {
    // 禁用全局中断
    __asm__ volatile("di");
}

static inline void irq_enable(void) {
    // 启用全局中断
    __asm__ volatile("ei");
}

#endif
EOF

echo "已创建 irq_wrapper.h"

# 4. 检查并修复pwm_driver.h中的语法错误
if grep -q "PWM_FAULT_OPEN circuit" pwm_driver.h; then
    sed -i 's/PWM_FAULT_OPEN circuit/PWM_FAULT_OPEN_CIRCUIT/g' pwm_driver.h
    echo "已修复 pwm_driver.h 中的语法错误"
fi

# 5. 创建按钮别名
if [ ! -f button_defs.h ]; then
cat > button_defs.h << 'EOF'
#ifndef __BUTTON_DEFS_H__
#define __BUTTON_DEFS_H__

#include "button_driver.h"

#define BTN_OK          BUTTON_OK_PIN
#define BTN_START       BUTTON_START_PIN
#define BTN_CANCEL      BUTTON_CANCEL_PIN
#define BTN_STOP        BUTTON_STOP_PIN
#define BTN_UP          BUTTON_UP_PIN
#define BTN_DOWN        BUTTON_DOWN_PIN

#endif
EOF
    echo "已创建 button_defs.h"
fi

# 6. 检查main_master_v3.c的include
grep -q "button_defs.h" main_master_v3.c || echo "注意: main_master_v3.c可能需要添加#include \"button_defs.h\""
grep -q "irq_wrapper.h" main_master_v3.c || echo "注意: main_master_v3.c可能需要添加#include \"irq_wrapper.h\""

echo ""
echo "=== 修复完成，尝试构建 ==="
cd /mnt/d/TLSR8269_SixChannel_Pulse_Generator/firmware
make clean
make 2>&1 | tee build.log

echo ""
echo "=== 构建结果 ==="
if [ -f PulseTherapy.bin ]; then
    echo "成功: PulseTherapy.bin 已生成"
    ls -lh PulseTherapy.bin
else
    echo "失败: 检查 build.log 获取详细信息"
    tail -50 build.log
fi
