# BARA ESP32 WiFi Security Tool - Advanced Makefile Build System
# ===============================================================
# Created by: أحمد نور أحمد من قنا
# Developer: Ahmed Nour Ahmed from Qena
#
# Advanced Makefile system for building, uploading, and managing
# ESP32 projects with BARA tool
#
# Usage:
#   make                    - Build the project
#   make upload             - Build and upload to ESP32
#   make monitor            - Build, upload and monitor serial output
#   make clean              - Clean build files
#   make config             - Show configuration
#   make debug              - Build with debug information
#   make test               - Run tests
#   make update-tools       - Update development tools
#
# Custom Configuration:
#   Edit makefile.config to modify default settings
#   Set environment variables to override defaults

# Project Configuration
PROJECT_NAME = bara
TARGET = esp32
BOARD = esp32dev
FRAMEWORK = arduino
PLATFORM = espressif32
UPLOAD_SPEED = 921600
MONITOR_SPEED = 115200
FLASH_SIZE = 4MB
CPU_FREQUENCY = 240MHz

# Source Files
SRCDIR = src
INCDIR = include
LIBDIR = lib
BUILDDIR = build
DEPLOYDIR = deploy

# Source Files
SOURCES = $(wildcard $(SRCDIR)/*.cpp)
HEADERS = $(wildcard $(INCDIR)/*.h)
LIBS = $(wildcard $(LIBDIR)/*.cpp)

# Platform-specific paths
ESP32_SDK_PATH = $(HOME)/.platformio/packages/framework-arduinoespressif32
ARDUINO_IDE_PATH = $(HOME)/Arduino
PLATFORMIO_BIN = $(HOME)/.platformio/penv/bin/pio

# Compiler Configuration
CC = xtensa-esp32-elf-g++
CXX = xtensa-esp32-elf-g++
AR = xtensa-esp32-elf-ar
SIZE = xtensa-esp32-elf-size
OBJDUMP = xtensa-esp32-elf-objdump
NM = xtensa-esp32-elf-nm

# Arduino Framework Paths
ARDUINO_LIB_PATH = $(ESP32_SDK_PATH)/libraries
ARDUINO_CORE_PATH = $(ESP32_SDK_PATH)/cores/esp32

# Build Flags
CFLAGS = -std=c11 -DARDUINO=10801 -DPLATFORMIO=60103
CXXFLAGS = -std=gnu++11 -DARDUINO=10801 -DPLATFORMIO=60103
CPPFLAGS = -fpermissive
INCLUDES = -I$(INCDIR) -I$(ARDUINO_CORE_PATH) -I$(ARDUINO_LIB_PATH)/WiFi/src -I$(ARDUINO_LIB_PATH)/WebServer/src -I$(ARDUINO_LIB_PATH)/SPIFFS/src
LDFLAGS = -nostartfiles -specs=nosys.specs -specs=nano.specs

# Optimization Flags
DEBUG_FLAGS = -O0 -g -DDEBUG_MODE=1
RELEASE_FLAGS = -Os -flto -DRELEASE_MODE=1
WARNINGS = -Wall -Wextra -pedantic

# Security Testing Flags
SECURITY_FLAGS = -DSECURITY_TESTING_ENABLED=1 -DADVANCED_WIFI_ANALYSIS=1 -DNETWORK_MONITORING=1

# Default Target
.DEFAULT_GOAL := all

# Include Configuration
-include makefile.config
-include makefile.tools
-include makefile.targets

# Include Custom Configuration
ifdef CUSTOM_CONFIG
-include $(CUSTOM_CONFIG)
endif

# Tool Detection
ifeq ($(shell which pio),)
  USE_PLATFORMIO = 0
  USE_ARDUINO_IDE = 1
else
  USE_PLATFORMIO = 1
  USE_ARDUINO_IDE = 0
endif

# Determine Operating System
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
  OS = linux
  SERIAL_PORT = /dev/ttyUSB0
elifeq ($(UNAME_S),Darwin)
  OS = macosx
  SERIAL_PORT = /dev/cu.usbserial*
else
  OS = windows
  SERIAL_PORT = COM3
endif

# Colors for terminal output
ifeq ($(shell tput colors 2>/dev/null),256)
  RED = \033[0;31m
  GREEN = \033[0;32m
  YELLOW = \033[0;33m
  BLUE = \033[0;34m
  MAGENTA = \033[0;35m
  CYAN = \033[0;36m
  WHITE = \033[0;37m
  RESET = \033[0m
  BOLD = \033[1m
else
  RED =
  GREEN =
  YELLOW =
  BLUE =
  MAGENTA =
  CYAN =
  WHITE =
  RESET =
  BOLD =
endif

# Main Build Targets
all: $(BUILDDIR)/$(PROJECT_NAME).bin $(BUILDDIR)/$(PROJECT_NAME).elf

# Create Build Directory
$(BUILDDIR):
	@echo "$(BLUE)Creating build directory...$(RESET)"
	@mkdir -p $(BUILDDIR)

# Compile Source Files
$(BUILDDIR)/%.o: $(SRCDIR)/%.cpp | $(BUILDDIR)
	@echo "$(CYAN)Compiling $<...$(RESET)"
	@$(CXX) $(CXXFLAGS) $(INCLUDES) $(WARNINGS) $(DEBUG_FLAGS) $(SECURITY_FLAGS) -c $< -o $@

$(BUILDDIR)/%.o: $(LIBDIR)/%.cpp | $(BUILDDIR)
	@echo "$(CYAN)Compiling $<...$(RESET)"
	@$(CXX) $(CXXFLAGS) $(INCLUDES) $(WARNINGS) $(DEBUG_FLAGS) $(SECURITY_FLAGS) -c $< -o $@

# Link Object Files
$(BUILDDIR)/$(PROJECT_NAME).elf: $(SOURCES:$(SRCDIR)/%.cpp=$(BUILDDIR)/%.o) $(LIBS:$(LIBDIR)/%.cpp=$(BUILDDIR)/%.o)
	@echo "$(GREEN)Linking $(PROJECT_NAME).elf...$(RESET)"
	@$(CXX) $(LDFLAGS) -o $@ $(BUILDDIR)/*.o -L$(ARDUINO_CORE_PATH) -lgcc -lhal -ld -lm -lnewlib -lc

# Generate Binary
$(BUILDDIR)/$(PROJECT_NAME).bin: $(BUILDDIR)/$(PROJECT_NAME).elf
	@echo "$(GREEN)Generating binary...$(RESET)"
	@$(SIZE) $^
	@esptool.py elf2image -o $(BUILDDIR)/ --version=2 $^

# Upload to ESP32
upload: $(BUILDDIR)/$(PROJECT_NAME).bin
	@echo "$(YELLOW)Uploading to ESP32...$(RESET)"
	@echo "$(YELLOW)Please press RESET button on ESP32...$(RESET)"
	@esptool.py --chip $(TARGET) --port $(SERIAL_PORT) --baud $(UPLOAD_SPEED) --before default_reset --after hard_reset --flash_mode dio --flash_freq 40m --flash_size 4MB 0x0000 $(BUILDDIR)/$(PROJECT_NAME).bin
	@echo "$(GREEN)Upload completed!$(RESET)"

# Build and Upload
build-upload: all upload

# Monitor Serial Output
monitor:
	@echo "$(BLUE)Starting serial monitor...$(RESET)"
	@echo "$(YELLOW)Press Ctrl+C to exit$(RESET)"
	@screen $(SERIAL_PORT) $(MONITOR_SPEED)

# Clean Build Files
clean:
	@echo "$(RED)Cleaning build files...$(RESET)"
	@rm -rf $(BUILDDIR)
	@echo "$(GREEN)Build directory cleaned!$(RESET)"

# Deep Clean (including Platform.io)
deep-clean: clean
	@echo "$(RED)Deep cleaning...$(RESET)"
	@rm -rf .pio
	@rm -f platformio.ini.bak
	@echo "$(GREEN)Deep clean completed!$(RESET)"

# Debug Build
debug: CXXFLAGS += -O0 -g -DDEBUG_MODE=1
debug: all

# Release Build
release: CXXFLAGS += $(RELEASE_FLAGS)
release: all

# Show Configuration
config:
	@echo "$(BOLD)BARA ESP32 Build Configuration:$(RESET)"
	@echo "$(WHITE)Project:$(RESET) $(PROJECT_NAME)"
	@echo "$(WHITE)Target:$(RESET) $(TARGET)"
	@echo "$(WHITE)Board:$(RESET) $(BOARD)"
	@echo "$(WHITE)Platform:$(RESET) $(PLATFORM)"
	@echo "$(WHITE)Upload Speed:$(RESET) $(UPLOAD_SPEED)"
	@echo "$(WHITE)Monitor Speed:$(RESET) $(MONITOR_SPEED)"
	@echo "$(WHITE)Serial Port:$(RESET) $(SERIAL_PORT)"
	@echo "$(WHITE)OS:$(RESET) $(OS)"
	@echo "$(WHITE)Use Platform.io:$(RESET) $(USE_PLATFORMIO)"
	@echo "$(WHITE)Build Directory:$(RESET) $(BUILDDIR)"
	@echo "$(WHITE)Source Directory:$(RESET) $(SRCDIR)"
	@echo "$(WHITE)Include Directory:$(RESET) $(INCDIR)"
	@echo "$(WHITE)Library Directory:$(RESET) $(LIBDIR)"

# Check System Requirements
check-deps:
	@echo "$(BLUE)Checking dependencies...$(RESET)"
	@echo "$(WHITE)ESP32 SDK:$(RESET) $(if $(wildcard $(ESP32_SDK_PATH)),$(GREEN)Found$(RESET),$(RED)Not Found$(RESET))"
	@echo "$(WHITE)Platform.io:$(RESET) $(if $(shell which pio),$(GREEN)Installed$(RESET),$(RED)Not Found$(RESET))"
	@echo "$(WHITE)Arduino IDE:$(RESET) $(if $(wildcard $(ARDUINO_IDE_PATH)),$(GREEN)Found$(RESET),$(RED)Not Found$(RESET))"
	@echo "$(WHITE)ESPTool:$(RESET) $(if $(shell which esptool.py),$(GREEN)Installed$(RESET),$(RED)Not Found$(RESET))"
	@echo "$(WHITE)Screen:$(RESET) $(if $(shell which screen),$(GREEN)Installed$(RESET),$(RED)Not Found$(RESET))"

# Install Dependencies
install-deps:
	@echo "$(BLUE)Installing dependencies...$(RESET)"
	@echo "$(YELLOW)Please run install-deps.sh or install-deps.bat for your platform$(RESET)"

# Update Toolchain
update-tools:
	@echo "$(BLUE)Updating ESP32 toolchain...$(RESET)"
	@echo "$(YELLOW)Platform.io users: Run 'pio update'$(RESET)"
	@echo "$(YELLOW)Arduino IDE users: Use Board Manager to update ESP32 package$(RESET)"

# Backup Configuration
backup-config:
	@echo "$(BLUE)Creating configuration backup...$(RESET)"
	@mkdir -p backup/$(shell date +%Y%m%d_%H%M%S)
	@cp makefile* backup/$(shell date +%Y%m%d_%H%M%S)/
	@cp -r $(SRCDIR) backup/$(shell date +%Y%m%d_%H%M%S)/
	@cp -r $(INCDIR) backup/$(shell date +%Y%m%d_%H%M%S)/
	@cp -r $(LIBDIR) backup/$(shell date +%Y%m%d_%H%M%S)/
	@echo "$(GREEN)Backup created in backup/$(shell date +%Y%m%d_%H%M%S)/$(RESET)"

# Generate Documentation
docs:
	@echo "$(BLUE)Generating documentation...$(RESET)"
	@doxygen Doxyfile
	@echo "$(GREEN)Documentation generated in docs/html/$(RESET)"

# Run Tests
test:
	@echo "$(BLUE)Running tests...$(RESET)"
	@echo "$(YELLOW)Test functionality not yet implemented$(RESET)"

# Flash Memory Analysis
memory-analysis: $(BUILDDIR)/$(PROJECT_NAME).elf
	@echo "$(BLUE)Analyzing memory usage...$(RESET)"
	@$(SIZE) $^

# Performance Analysis
perf-analysis: $(BUILDDIR)/$(PROJECT_NAME).elf
	@echo "$(BLUE)Performing performance analysis...$(RESET)"
	@$(OBJDUMP) -h $^

# Symbol Analysis
symbols: $(BUILDDIR)/$(PROJECT_NAME).elf
	@echo "$(BLUE)Analyzing symbols...$(RESET)"
	@$(NM) $^ | head -20

# Help Target
help:
	@echo "$(BOLD)BARA ESP32 Build System$(RESET)"
	@echo "=========================="
	@echo "$(GREEN)Build Targets:$(RESET)"
	@echo "  all             - Build the project"
	@echo "  debug           - Build with debug information"
	@echo "  release         - Build with optimization"
	@echo ""
	@echo "$(GREEN)Upload Targets:$(RESET)"
	@echo "  upload          - Upload to ESP32"
	@echo "  build-upload    - Build and upload"
	@echo ""
	@echo "$(GREEN)Monitor Targets:$(RESET)"
	@echo "  monitor         - Monitor serial output"
	@echo ""
	@echo "$(GREEN)Utility Targets:$(RESET)"
	@echo "  clean           - Clean build files"
	@echo "  deep-clean      - Clean all files"
	@echo "  config          - Show configuration"
	@echo "  check-deps      - Check dependencies"
	@echo "  backup-config   - Backup configuration"
	@echo "  docs            - Generate documentation"
	@echo ""
	@echo "$(GREEN)Analysis Targets:$(RESET)"
	@echo "  memory-analysis - Analyze memory usage"
	@echo "  perf-analysis   - Performance analysis"
	@echo "  symbols         - Symbol analysis"
	@echo ""
	@echo "$(GREEN)Maintenance Targets:$(RESET)"
	@echo "  install-deps    - Install dependencies"
	@echo "  update-tools    - Update toolchain"
	@echo "  test            - Run tests"
	@echo ""
	@echo "$(WHITE)For more information, see README.md$(RESET)"

# Variables that change
override CXXFLAGS += $(WARNINGS)

# PHONY targets
.PHONY: all clean deep-clean debug release config check-deps install-deps update-tools backup-config docs test monitor memory-analysis perf-analysis symbols help upload build-upload

# Include platform-specific makefiles
-include $(OS)/makefile.$(OS)

# Custom Targets Section
# Add your custom targets below this line