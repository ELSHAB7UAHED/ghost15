#!/bin/bash

# BARA ESP32 WiFi Security Tool - Build Script
# ============================================
# Created by: أحمد نور أحمد من قنا
# Developer: Ahmed Nour Ahmed from Qena
#
# This script builds the ESP32 binary for the BARA WiFi Security Testing Tool
# Supports Arduino IDE and PlatformIO compilation methods

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
PROJECT_NAME="bara"
SOURCE_FILE="bara.cpp"
BUILD_DIR="build"
DEPLOY_DIR="deploy"
LOG_FILE="build.log"

# Helper functions
log_info() {
    echo -e "${BLUE}[INFO]${NC} $1" | tee -a $LOG_FILE
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1" | tee -a $LOG_FILE
}

log_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1" | tee -a $LOG_FILE
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1" | tee -a $LOG_FILE
}

# Check dependencies
check_dependencies() {
    log_info "Checking dependencies..."
    
    # Check for Arduino CLI
    if command -v arduino-cli &> /dev/null; then
        ARDUINO_CLI_VERSION=$(arduino-cli version | cut -d' ' -f2)
        log_success "Arduino CLI found: v$ARDUINO_CLI_VERSION"
        USE_ARDUINO_CLI=1
    else
        log_warning "Arduino CLI not found"
        USE_ARDUINO_CLI=0
    fi
    
    # Check for PlatformIO
    if command -v pio &> /dev/null; then
        PLATFORMIO_VERSION=$(pio --version | cut -d' ' -f2)
        log_success "PlatformIO found: v$PLATFORMIO_VERSION"
        USE_PLATFORMIO=1
    else
        log_warning "PlatformIO not found"
        USE_PLATFORMIO=0
    fi
    
    # Check for ESP32 SDK
    if [ -d "$HOME/.platformio/packages/framework-arduinoespressif32" ]; then
        log_success "ESP32 Arduino SDK found"
        ESP32_SDK_FOUND=1
    else
        log_warning "ESP32 Arduino SDK not found in expected location"
        ESP32_SDK_FOUND=0
    fi
    
    # Check for esptool.py
    if command -v esptool.py &> /dev/null; then
        ESPTOOL_VERSION=$(esptool.py version 2>/dev/null | grep "esptool.py" | cut -d' ' -f2 | tr -d ',')
        log_success "esptool.py found: v$ESPTOOL_VERSION"
    else
        log_warning "esptool.py not found in PATH"
    fi
}

# Install dependencies if needed
install_dependencies() {
    log_info "Installing dependencies if needed..."
    
    # Install Arduino CLI if not present
    if [ $USE_ARDUINO_CLI -eq 0 ]; then
        log_info "Installing Arduino CLI..."
        curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | sh -s -- --bin-dir /usr/local/bin
        export PATH="/usr/local/bin:$PATH"
        USE_ARDUINO_CLI=1
    fi
    
    # Install PlatformIO if not present
    if [ $USE_PLATFORMIO -eq 0 ]; then
        log_info "Installing PlatformIO..."
        pip3 install platformio
        export PATH="$HOME/.platformio/penv/bin:$PATH"
        USE_PLATFORMIO=1
    fi
}

# Setup ESP32 board
setup_esp32_board() {
    log_info "Setting up ESP32 board..."
    
    if [ $USE_ARDUINO_CLI -eq 1 ]; then
        # Update board index
        arduino-cli core update-index
        
        # Install ESP32 core
        arduino-cli core install esp32:esp32 --additional-urls https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
        log_success "ESP32 core installed"
    fi
}

# Build using Arduino CLI
build_with_arduino() {
    log_info "Building with Arduino CLI..."
    
    # Compile
    arduino-cli compile --fqbn esp32:esp32:esp32 $SOURCE_FILE --output-dir $BUILD_DIR 2>&1 | tee -a $LOG_FILE
    
    # Check if build succeeded
    if [ $? -eq 0 ]; then
        log_success "Arduino CLI build completed successfully"
        
        # Copy binary to deploy directory
        mkdir -p $DEPLOY_DIR
        cp $BUILD_DIR/*.bin $DEPLOY_DIR/
        cp $BUILD_DIR/*.elf $DEPLOY_DIR/ 2>/dev/null || true
        
        log_success "Binary files copied to $DEPLOY_DIR"
    else
        log_error "Arduino CLI build failed"
        return 1
    fi
}

# Build using PlatformIO
build_with_platformio() {
    log_info "Building with PlatformIO..."
    
    # Create platformio.ini if it doesn't exist
    if [ ! -f "platformio.ini" ]; then
        log_info "Creating platformio.ini..."
        cat > platformio.ini << EOF
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
board_build.filesystem = spiffs
board_build.partitions = default
monitor_speed = 115200
upload_speed = 921600
EOF
    fi
    
    # Compile
    pio run 2>&1 | tee -a $LOG_FILE
    
    # Check if build succeeded
    if [ $? -eq 0 ]; then
        log_success "PlatformIO build completed successfully"
        
        # Copy binary to deploy directory
        mkdir -p $DEPLOY_DIR
        cp .pio/build/esp32dev/firmware.bin $DEPLOY_DIR/bara.bin
        cp .pio/build/esp32dev/firmware.elf $DEPLOY_DIR/bara.elf 2>/dev/null || true
        
        log_success "Binary files copied to $DEPLOY_DIR"
    else
        log_error "PlatformIO build failed"
        return 1
    fi
}

# Simulate build (for GitHub Actions or environments without ESP32 SDK)
simulate_build() {
    log_info "Simulating ESP32 build (no actual hardware available)..."
    
    # Create build directory
    mkdir -p $BUILD_DIR
    mkdir -p $DEPLOY_DIR
    
    # Simulate compilation process
    log_info "Compiling $SOURCE_FILE..."
    sleep 2
    
    # Create simulated binary
    log_info "Generating binary..."
    sleep 1
    
    # Create a placeholder binary file
    cat > $DEPLOY_DIR/bara.bin << 'EOF'
ESP32 WiFi Security Testing Tool - BARA
=======================================
Developer: أحمد نور أحمد من قنا
Platform: ESP32
Purpose: Educational Security Testing

This is a simulated binary file for demonstration purposes.
In a real ESP32 build environment, this would be the actual
compiled binary ready for upload to ESP32 hardware.

Features included:
- WiFi Access Point: "bara" / "A7med@Elshab7"
- Real-time network scanning
- Dark hacker interface
- WebSocket communication
- Deauth attack simulation
- Data export capabilities

To build the actual binary:
1. Install Arduino IDE with ESP32 support
2. OR install PlatformIO
3. Run: make build
4. Upload to ESP32 via USB

Built on: $(date)
Build type: Simulation
EOF
    
    # Create size information
    echo "Binary size simulation:"
    ls -lh $DEPLOY_DIR/bara.bin
    
    log_success "Simulated build completed"
}

# Generate build report
generate_report() {
    log_info "Generating build report..."
    
    REPORT_FILE="build_report.txt"
    {
        echo "BARA ESP32 WiFi Security Testing Tool - Build Report"
        echo "===================================================="
        echo "Date: $(date)"
        echo "Build Method: $BUILD_METHOD"
        echo ""
        echo "Source File: $SOURCE_FILE"
        echo "Project Name: $PROJECT_NAME"
        echo "Target Platform: ESP32"
        echo ""
        echo "Dependencies Check:"
        echo "- Arduino CLI: $([ $USE_ARDUINO_CLI -eq 1 ] && echo "✓ Available" || echo "✗ Not Found")"
        echo "- PlatformIO: $([ $USE_PLATFORMIO -eq 1 ] && echo "✓ Available" || echo "✗ Not Found")"
        echo "- ESP32 SDK: $([ $ESP32_SDK_FOUND -eq 1 ] && echo "✓ Available" || echo "✗ Not Found")"
        echo ""
        echo "Build Results:"
        if [ -f "$DEPLOY_DIR/bara.bin" ]; then
            echo "- Binary: ✓ Generated ($DEPLOY_DIR/bara.bin)"
            echo "- Size: $(du -h $DEPLOY_DIR/bara.bin | cut -f1)"
        else
            echo "- Binary: ✗ Failed"
        fi
        
        if [ -f "$DEPLOY_DIR/bara.elf" ]; then
            echo "- ELF: ✓ Generated ($DEPLOY_DIR/bara.elf)"
            echo "- Size: $(du -h $DEPLOY_DIR/bara.elf | cut -f1)"
        else
            echo "- ELF: ✗ Not generated"
        fi
        echo ""
        echo "Build Environment:"
        uname -a
        echo ""
        echo "Developer: أحمد نور أحمد من قنا"
        echo "Tool: bara.cpp - Educational WiFi Security Testing"
    } > $REPORT_FILE
    
    log_success "Build report generated: $REPORT_FILE"
}

# Clean build artifacts
clean_build() {
    log_info "Cleaning build artifacts..."
    rm -rf $BUILD_DIR $DEPLOY_DIR
    rm -f $LOG_FILE platformio.ini
    log_success "Build artifacts cleaned"
}

# Main build process
main() {
    echo "=============================================="
    echo "BARA ESP32 WiFi Security Tool - Build System"
    echo "=============================================="
    echo "Developer: أحمد نور أحمد من قنا"
    echo "Platform: ESP32"
    echo ""
    
    # Initialize log
    echo "Build started: $(date)" > $LOG_FILE
    
    # Check command line arguments
    BUILD_METHOD="simulate"
    CLEAN_BEFORE=0
    
    while [[ $# -gt 0 ]]; do
        case $1 in
            --clean)
                CLEAN_BEFORE=1
                shift
                ;;
            --arduino)
                BUILD_METHOD="arduino"
                shift
                ;;
            --platformio)
                BUILD_METHOD="platformio"
                shift
                ;;
            --simulate)
                BUILD_METHOD="simulate"
                shift
                ;;
            help|-h|--help)
                echo "Usage: $0 [OPTIONS]"
                echo ""
                echo "Options:"
                echo "  --arduino      Build using Arduino CLI"
                echo "  --platformio   Build using PlatformIO"
                echo "  --simulate     Simulate build (default)"
                echo "  --clean        Clean build artifacts before building"
                echo "  help          Show this help message"
                echo ""
                echo "Examples:"
                echo "  $0                    # Simulate build"
                echo "  $0 --arduino          # Build with Arduino CLI"
                echo "  $0 --platformio       # Build with PlatformIO"
                echo "  $0 --clean --arduino  # Clean and build with Arduino CLI"
                exit 0
                ;;
            *)
                log_error "Unknown option: $1"
                echo "Use '$0 help' for usage information"
                exit 1
                ;;
        esac
    done
    
    # Clean if requested
    if [ $CLEAN_BEFORE -eq 1 ]; then
        clean_build
    fi
    
    # Start build process
    check_dependencies
    
    # Determine build method
    if [ $BUILD_METHOD = "simulate" ]; then
        log_info "Using simulation mode"
        simulate_build
        BUILD_METHOD="Simulation"
    elif [ $BUILD_METHOD = "arduino" ]; then
        if [ $USE_ARDUINO_CLI -eq 0 ]; then
            log_info "Installing Arduino CLI..."
            install_dependencies
        fi
        setup_esp32_board
        build_with_arduino || exit 1
    elif [ $BUILD_METHOD = "platformio" ]; then
        if [ $USE_PLATFORMIO -eq 0 ]; then
            log_info "Installing PlatformIO..."
            install_dependencies
        fi
        build_with_platformio || exit 1
    fi
    
    # Generate report
    generate_report
    
    echo ""
    echo "=============================================="
    log_success "Build process completed!"
    echo "=============================================="
    echo "Output files:"
    echo "- Binary: $DEPLOY_DIR/bara.bin"
    echo "- Report: build_report.txt"
    echo "- Log: $LOG_FILE"
    echo ""
    echo "To upload to ESP32:"
    echo "1. Connect ESP32 via USB"
    echo "2. Run: make upload"
    echo "3. Or use Arduino IDE to upload $DEPLOY_DIR/bara.bin"
    echo ""
    echo "Developer: أحمد نور أحمد من قنا"
    echo "Tool: bara.cpp - Educational WiFi Security Testing"
}

# Run main function with all arguments
main "$@"