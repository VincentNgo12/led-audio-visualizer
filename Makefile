# -----------------------------------------------------------------------------
# Makefile for STM32F103 baremetal project
# -----------------------------------------------------------------------------

DEBUG ?= 0  # Default to not debug mode

# Project name (no extension)
PROGRAM = led_audio_visualizer

# Compiler and flags
CC = arm-none-eabi-gcc
CFLAGS = -mcpu=cortex-m3 -mthumb -nostdlib -Wall -Werror -Os -flto
CFLAGS += -DARM_MATH_CM3 #-Ofast -ffast-math #Additional flags for CMSIS-DSP 
CPPFLAGS = -DSTM32F103xB \
	-Ivendor/CMSIS/Device/ST/STM32F1/Include \
	-Ivendor/CMSIS/CMSIS/Core/Include \
	-Ivendor/CMSIS/CMSIS/DSP/Include \
	-Ivendor/CMSIS/CMSIS/DSP/PrivateInclude \
	-Isrc \
	-Iinclude

ifeq ($(DEBUG),1)
  CFLAGS += -g3 -Og -fno-inline # Add debug flags if in debug mode
endif

# Linker file
LINKER_FILE = linker_script.ld
LDFLAGS = -T $(LINKER_FILE)


# Get all .c files in src/ automatically
SRC = $(wildcard src/*.c)

#Path to CMSIS-DSP
CMSIS-DSP = ./vendor/CMSIS/CMSIS/DSP
#Additional CMSIS Source Files
SRC += ./vendor/CMSIS/Device/ST/STM32F1/Source/Templates/system_stm32f1xx.c \
		$(CMSIS-DSP)/Source/TransformFunctions/arm_cfft_radix4_q15.c \
		$(CMSIS-DSP)/Source/ComplexMathFunctions/arm_cmplx_mag_q15.c \
		$(CMSIS-DSP)/Source/CommonTables/arm_common_tables.c \
		$(CMSIS-DSP)/Source/CommonTables/arm_const_structs.c \
		$(CMSIS-DSP)/Source/SupportFunctions/arm_bitreversal.c \
		$(CMSIS-DSP)/Source/FastMathFunctions/arm_sqrt_q31.c \

BUILD_DIR = ./build

# Object files will all go into the build/ directory
OBJ = $(addprefix $(BUILD_DIR)/, $(notdir $(SRC:.c=.o)))

vpath %.c $(sort $(dir $(SRC))) #Find Source Files across Directories


# -----------------------------------------------------------------------------
# Build rules
# -----------------------------------------------------------------------------
all: $(PROGRAM).elf

# Link object files into final ELF
$(PROGRAM).elf: $(OBJ)
	$(CC) $(CFLAGS) $(CPPFLAGS) $(LDFLAGS) $^ -o $(PROGRAM).elf

#Compile each C files into obj file (inside build folder).
$(BUILD_DIR)/%.o: %.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

# -----------------------------------------------------------------------------
# Cleaning
# -----------------------------------------------------------------------------
.PHONY: clean

clean:
	rm -f $(BUILD_DIR)/*.o *.elf
# -----------------------------------------------------------------------------
# Flashing to target (using OpenOCD and ST-Link)
# -----------------------------------------------------------------------------
PROGRAMMER=openocd
PROGRAMMER_FLAGS=-f /usr/share/openocd/scripts/interface/stlink-v2.cfg -f /usr/share/openocd/scripts/target/stm32f1x.cfg

flash: $(PROGRAM).elf
	$(PROGRAMMER) $(PROGRAMMER_FLAGS) -c "program $(PROGRAM).elf verify reset exit"


# -----------------------------------------------------------------------------
# Debug Rules
# -----------------------------------------------------------------------------

# -----------------------------------------------------------------------------
# Debug Mode
# -----------------------------------------------------------------------------
$(PROGRAM)-debug.elf: $(OBJ) $(BUILD_DIR)/system_stm32f1xx.o
	$(CC) $(CFLAGS) $(CPPFLAGS) $(LDFLAGS) -g3 -Og $^ -o $(PROGRAM)-debug.elf

.PHONY: debug
debug:
	$(MAKE) DEBUG=1 $(PROGRAM)-debug.elf 
	@echo "Starting OpenOCD..."
	openocd -f /usr/share/openocd/scripts/interface/stlink-v2.cfg -f /usr/share/openocd/scripts/target/stm32f1x.cfg & \
	PID=$$!; \
	sleep 1; \
	gdb-multiarch -ex "target extended-remote :3333" -ex "monitor reset halt" $(PROGRAM)-debug.elf; \
	kill $$PID