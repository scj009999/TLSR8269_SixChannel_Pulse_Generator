# TLSR8269 六通道脉冲治疗仪 - Makefile
# 
# 使用说明：
#   make          - 编译项目
#   make clean    - 清理编译文件
#   make flash    - 烧录到设备
#   make size     - 显示内存使用
#
# 环境变量：
#   TELINK_SDK    - Telink SDK路径
#   PORT          - 串口号（Windows: COM3, Linux: /dev/ttyUSB0）

# 项目配置
PROJECT_NAME = PulseTherapy
TARGET = $(PROJECT_NAME)

# SDK路径（可通过环境变量覆盖）
TELINK_SDK ?= $(TELINK_SDK_PATH)
ifeq ($(TELINK_SDK),)
    $(error TELINK_SDK not set. Please set TELINK_SDK_PATH environment variable)
endif

# 工具链
TOOLCHAIN_PREFIX = tc32-elf-
CC = $(TOOLCHAIN_PREFIX)gcc
LD = $(TOOLCHAIN_PREFIX)ld
OBJCOPY = $(TOOLCHAIN_PREFIX)objcopy
OBJDUMP = $(TOOLCHAIN_PREFIX)objdump
SIZE = $(TOOLCHAIN_PREFIX)size
AR = $(TOOLCHAIN_PREFIX)ar

# 编译选项
CFLAGS = -ffunction-sections -fdata-sections
CFLAGS += -Wall -Wextra
CFLAGS += -std=gnu99
CFLAGS += -O2
CFLAGS += -g  # 调试信息

# 芯片定义
CFLAGS += -DCHIP_TYPE=CHIP_TYPE_8269
CFLAGS += -DMCU_CORE_8269

# 包含路径
CFLAGS += -I$(TELINK_SDK)/drivers/8269
CFLAGS += -I$(TELINK_SDK)/drivers/common
CFLAGS += -I$(TELINK_SDK)/proj
CFLAGS += -I.

# 链接选项
LDFLAGS = -T$(TELINK_SDK)/boot/link/boot_8269.link
LDFLAGS += --gc-sections
LDFLAGS += -Map=$(TARGET).map

# 源文件
SRCS = main_master_v3.c \
       timer_driver.c \
       uart_driver.c \
       i2c_driver.c \
       pwm_driver.c \
       adc_driver.c \
       lcd_driver.c \
       button_driver.c \
       ui_manager.c \
       self_check.c \
       stubs.c

# SDK源文件（根据需要添加）
SDK_SRCS = $(TELINK_SDK)/drivers/8269/gpio_8269.c \
           $(TELINK_SDK)/drivers/8269/clock_8269.c \
           $(TELINK_SDK)/drivers/8269/timer_8269.c

# 所有源文件
ALL_SRCS = $(SRCS) $(SDK_SRCS)

# 目标文件
OBJS = $(SRCS:.c=.o)
SDK_OBJS = $(SDK_SRCS:.c=.o)
ALL_OBJS = $(OBJS) $(SDK_OBJS)

# 依赖文件
DEPS = $(ALL_SRCS:.c=.d)

# 烧录配置
PORT ?= COM3
BAUD ?= 921600

# 规则
.PHONY: all clean flash size debug release

all: $(TARGET).bin
	@echo "========================================"
	@echo "Build complete: $(TARGET).bin"
	@echo "========================================"
	@$(SIZE) $(TARGET).elf
	@echo "========================================"

$(TARGET).elf: $(ALL_OBJS)
	@echo "LINK $@"
	@$(LD) $(LDFLAGS) -o $@ $^

$(TARGET).bin: $(TARGET).elf
	@echo "BIN $@"
	@$(OBJCOPY) -O binary $< $@
	@$(OBJCOPY) -O ihex $< $(TARGET).hex

%.o: %.c
	@echo "CC $<"
	@$(CC) $(CFLAGS) -MMD -MP -c -o $@ $<

# 包含依赖文件
-include $(DEPS)

clean:
	@echo "Cleaning..."
	@rm -f $(OBJS) $(SDK_OBJS) $(DEPS)
	@rm -f $(TARGET).elf $(TARGET).bin $(TARGET).hex $(TARGET).map
	@rm -f $(TARGET).lst $(TARGET).sym
	@echo "Clean complete"

flash: $(TARGET).bin
	@echo "Flashing to $(PORT)..."
	@python3 $(TELINK_SDK)/tools/bdt/bdt.py \
		flash \
		-p $(PORT) \
		-b $(BAUD) \
		-i $(TARGET).bin
	@echo "Flash complete"

size: $(TARGET).elf
	@echo "Memory usage:"
	@$(SIZE) -A -x $(TARGET).elf
	@echo ""
	@echo "Flash usage:"
	@$(SIZE) -B $(TARGET).elf

debug: CFLAGS += -DDEBUG -O0
debug: all

release: CFLAGS += -DRELEASE -Os
release: clean all
	@mkdir -p release
	@cp $(TARGET).bin release/
	@cp $(TARGET).hex release/
	@echo "Release files in release/"

# 反汇编
$(TARGET).lst: $(TARGET).elf
	@$(OBJDUMP) -d -S $< > $@

list: $(TARGET).lst
	@echo "Disassembly: $(TARGET).lst"

# 帮助
help:
	@echo "TLSR8269 Pulse Therapy - Build System"
	@echo ""
	@echo "Targets:"
	@echo "  make        - Build the project"
	@echo "  make clean  - Clean build files"
	@echo "  make flash  - Flash to device (PORT=$(PORT))"
	@echo "  make size   - Show memory usage"
	@echo "  make debug  - Build with debug info"
	@echo "  make release- Build optimized release"
	@echo "  make list   - Generate disassembly"
	@echo "  make help   - Show this help"
	@echo ""
	@echo "Variables:"
	@echo "  TELINK_SDK  - Telink SDK path"
	@echo "  PORT        - Serial port for flashing"
	@echo "  BAUD        - Baud rate for flashing"
