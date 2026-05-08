# TLSR8269 六路脉冲发生器 - Makefile
# 适用于 Telink TLSR826x SDK

# 芯片型号
CHIP_TYPE = TLSR8269

# 工具链
TC32_PATH = $(TELINK_SDK_PATH)/tools/linux/tc32
CC = $(TC32_PATH)/bin/tc32-elf-gcc
LD = $(TC32_PATH)/bin/tc32-elf-ld
OBJCOPY = $(TC32_PATH)/bin/tc32-elf-objcopy
OBJDUMP = $(TC32_PATH)/bin/tc32-elf-objdump
SIZE = $(TC32_PATH)/bin/tc32-elf-size

# SDK路径
TELINK_SDK_PATH ?= /opt/telink_sdk
SDK_PATH = $(TELINK_SDK_PATH)/ble_sdk_multimode
PROJ_PATH = $(SDK_PATH)/proj
VENDOR_PATH = .

# 包含路径
INCLUDES = \
    -I$(PROJ_PATH) \
    -I$(PROJ_PATH)/drivers \
    -I$(PROJ_PATH)/common \
    -I$(SDK_PATH)/stack/ble \
    -I$(VENDOR_PATH)

# 编译选项
CFLAGS = \
    -ffunction-sections \
    -fdata-sections \
    -Wall \
    -O2 \
    -fpack-struct \
    -fshort-enums \
    -finline-small-functions \
    -std=gnu99 \
    -fshort-wchar \
    -fms-extensions \
    -nostdlib \
    -D$(CHIP_TYPE) \
    $(INCLUDES)

# 链接选项
LDFLAGS = \
    -T$(PROJ_PATH)/boot/link/boot.link \
    -nostartfiles \
    -Wl,--gc-sections \
    -Wl,-Map=$(BUILD_DIR)/firmware.map

# 源文件
SRCS = \
    $(VENDOR_PATH)/main.c \
    $(PROJ_PATH)/drivers/uart.c \
    $(PROJ_PATH)/drivers/gpio.c \
    $(PROJ_PATH)/drivers/pwm.c \
    $(PROJ_PATH)/drivers/clock.c \
    $(PROJ_PATH)/drivers/timer.c \
    $(PROJ_PATH)/common/utility.c

# 构建目录
BUILD_DIR = build
OBJS = $(SRCS:%.c=$(BUILD_DIR)/%.o)

# 目标文件
TARGET = $(BUILD_DIR)/firmware
BIN = $(TARGET).bin
ELF = $(TARGET).elf

# 默认目标
.PHONY: all clean flash

all: $(BIN)

# 创建构建目录
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# 编译规则
$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# 链接
$(ELF): $(OBJS)
	$(CC) $(LDFLAGS) $^ -o $@

# 生成二进制文件
$(BIN): $(ELF)
	$(OBJCOPY) -O binary $< $@
	@echo "========================================"
	@echo "Build completed: $@"
	@echo "========================================"
	$(SIZE) $(ELF)

# 清理
clean:
	rm -rf $(BUILD_DIR)

# 烧录 (需要Telink BDT工具)
flash: $(BIN)
	@echo "请使用 Telink Burning and Debugging Tool 烧录 $(BIN)"

# 调试信息
debug:
	@echo "SDK_PATH: $(SDK_PATH)"
	@echo "SRCS: $(SRCS)"
	@echo "OBJS: $(OBJS)"
