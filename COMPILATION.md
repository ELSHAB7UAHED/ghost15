# bara.cpp - Compilation and Setup Guide

## Quick Start Guide

### 1. Arduino IDE Setup

#### Step 1: Install Arduino IDE
```bash
# Download Arduino IDE 2.x from official website
https://www.arduino.cc/en/software
```

#### Step 2: Install ESP32 Board Package
1. Open Arduino IDE
2. Go to File → Preferences
3. Add ESP32 board URL:
   ```
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```
4. Go to Tools → Board → Boards Manager
5. Search for "ESP32" and install "esp32 by Espressif Systems"

#### Step 3: Select Board
- Tools → Board → esp32 → "ESP32 Dev Module"

#### Step 4: Configure Board Settings
```
Board: "ESP32 Dev Module"
Upload Speed: "921600"
CPU Frequency: "240MHz (WiFi/BT)"
Flash Frequency: "80MHz"
Flash Mode: "QIO"
Flash Size: "4MB (32Mb)"
Partition Scheme: "Default 4MB with spiffs"
Core Debug Level: "None"
```

### 2. Library Installation

#### Required Libraries (Usually pre-installed with ESP32 core)
- WiFi.h ✓
- WebServer.h ✓
- ArduinoJson ✓ (may need manual installation)
- ESPAsyncWebSrv.h (install separately if needed)

#### Install ArduinoJson (if needed)
1. Sketch → Include Library → Manage Libraries
2. Search "ArduinoJson"
3. Install version 6.x or higher

#### Install ESPAsyncWebSrv (if needed)
1. Download from: https://github.com/me-no-dev/ESPAsyncWebServer
2. Sketch → Include Library → Add .ZIP Library
3. Select downloaded ZIP file

### 3. Compilation Steps

#### Method 1: Arduino IDE
1. Open bara.cpp in Arduino IDE
2. Connect ESP32 via USB
3. Select correct COM port
4. Click Upload (Ctrl+U)

#### Method 2: PlatformIO (VS Code)
1. Install PlatformIO Extension in VS Code
2. Create new ESP32 project
3. Replace main.cpp with bara.cpp content
4. Build and upload

### 4. Hardware Setup

#### Required Connections
```
ESP32 Pin    | Connection
-----------  | ------------
3.3V         | VCC
GND          | Ground  
USB          | Programming Power
GPIO 2       | Status LED (internal)
GPIO 0       | Boot/Reset Button (optional)
```

#### Optional Connections
```
Component    | ESP32 Pin | Purpose
------------|-----------|----------
Buzzer       | GPIO 18   | Audio alerts
LCD Display  | I2C Pins  | Status display
External LED | GPIO 2    | System status
Button       | GPIO 0    | Manual trigger
```

### 5. Deployment Steps

#### Step 1: Upload Code
```bash
# Compile and upload
arduino-cli compile --fqbn esp32:esp32:esp32 bara.cpp
arduino-cli upload -p /dev/ttyUSB0 --fqbn esp32:esp32:esp32 bara.cpp
```

#### Step 2: Connect to Access Point
1. Scan for WiFi networks
2. Connect to "bara" network
3. Password: "A7med@Elshab7"

#### Step 3: Access Web Interface
1. Open browser
2. Go to: http://192.168.4.1
3. Interface should load automatically

### 6. Testing and Validation

#### Basic Function Tests
1. ✅ LED blinking (system status)
2. ✅ WiFi Access Point active
3. ✅ Web server responding
4. ✅ WiFi scan working
5. ✅ WebSocket connection established

#### Advanced Feature Tests
1. ✅ Network detection
2. ✅ Signal strength measurement
3. ✅ Security type identification
4. ✅ Real-time data updates
5. ✅ Data export functionality

### 7. Troubleshooting

#### Common Compilation Errors

##### "WiFi.h: No such file"
```cpp
// Solution: Ensure ESP32 board package is installed
Tools → Board → Boards Manager → Install ESP32
```

##### "ArduinoJson: No such file"
```cpp
// Solution: Install ArduinoJson library
Sketch → Include Library → Manage Libraries → ArduinoJson
```

##### "WebServer.h: Multiple definitions"
```cpp
// Solution: Use correct WebServer header for ESP32
#include <WebServer.h>  // Not <ESP8266WebServer.h>
```

##### Memory Issues
```cpp
// Solution: Enable PSRAM if available
Tools → PSRAM → Enabled (if board supports)
```

#### Runtime Issues

##### Access Point Not Visible
- Check power supply (minimum 2A recommended)
- Verify GPIO configuration
- Reset ESP32

##### No WebSocket Connection
- Check browser compatibility
- Try different browser
- Clear browser cache

##### Scan Returns No Results
- Ensure ESP32 has clear view of 2.4GHz networks
- Move closer to networks
- Check WiFi module status

### 8. Performance Optimization

#### Memory Optimization
```cpp
// Reduce buffer sizes for memory-constrained devices
const int MAX_NETWORKS = 30;  // Reduced from 50

// Optimize JSON document sizes
DynamicJsonDocument doc(2048);  // Adjust based on available RAM
```

#### Speed Optimization
```cpp
// Reduce scan interval for faster updates
const int SCAN_INTERVAL = 2000;  // 2 seconds instead of 5

// Use async scanning where possible
WiFi.scanNetworks(true);  // Async mode
```

#### Power Optimization
```cpp
// Disable unnecessary modules
WiFi.setSleepMode(WIFI_LIGHT_SLEEP);

// Reduce LED blink frequency
ledTicker.attach(2.0, updateLED);  // Every 2 seconds
```

### 9. Advanced Configuration

#### Custom AP Settings
```cpp
// Modify access point configuration
const char* AP_NAME = "YourCustomName";
const char* AP_PASSWORD = "YourPassword";
const char* ADMIN_PASSWORD = "admin123";

// Change web server port
const int WEB_PORT = 8080;
```

#### Network Scan Optimization
```cpp
// Custom scan parameters
WiFi.scanNetworks(true, true);  // async = true, hidden = true

// Channel scanning
for (int channel = 1; channel <= 13; channel++) {
    WiFi.setChannel(channel);
    performWiFiScan();
}
```

### 10. Security Considerations

#### Network Security
- Change default passwords
- Use HTTPS for web interface (advanced)
- Implement authentication (planned feature)

#### Data Protection
- Do not store sensitive network data
- Clear memory after scans
- Use secure WebSocket connections

#### Legal Compliance
- Ensure authorized testing only
- Document consent for network testing
- Follow local cybersecurity laws

### 11. Monitoring and Debugging

#### Serial Monitor
```bash
# Open serial monitor for debugging
Monitor → Serial Monitor (115200 baud)
```

#### Debug Output
```cpp
// Enable debug logging
#define DEBUG_MODE 1

#if DEBUG_MODE
#define DEBUG_PRINT(x) Serial.print(x)
#define DEBUG_PRINTLN(x) Serial.println(x)
#else
#define DEBUG_PRINT(x)
#define DEBUG_PRINTLN(x)
#endif
```

#### Web Interface Logs
- Terminal window shows real-time logs
- WebSocket messages for frontend updates
- Export functionality for detailed analysis

### 12. Support and Community

#### Getting Help
1. Check serial monitor for errors
2. Verify hardware connections
3. Review this documentation
4. Contact developer: أحمد نور أحمد من قنا

#### Reporting Issues
Include in issue reports:
- ESP32 board model
- Arduino IDE version
- Library versions
- Serial monitor output
- Steps to reproduce

#### Contributing
- Test on multiple ESP32 variants
- Optimize performance
- Add new features
- Improve documentation

---

**Remember**: This tool is for educational purposes. Always ensure proper authorization before testing any networks.