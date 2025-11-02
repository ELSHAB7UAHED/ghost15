/*
 * bara.cpp - ESP32 WiFi Security Testing Tool
 * Created by: أحمد نور أحمد من قنا
 * Platform: ESP32
 * Interface: English
 * Purpose: Educational WiFi Security Testing & Analysis
 * 
 * Legal Notice: This tool is for educational purposes only.
 * Always ensure proper authorization before testing any networks.
 * Unauthorized access is illegal and unethical.
 */

#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <ESPAsyncWebSrv.h>
#include <SPIFFS.h>
#include <FS.h>
#include <DNSServer.h>
#include <WiFiManager.h>
#include <Ticker.h>
#include <FS.h>

// Configuration
const char* AP_NAME = "bara";
const char* AP_PASSWORD = "A7med@Elshab7";
const char* ADMIN_PASSWORD = "admin123";
const int WEB_PORT = 80;
const int SCAN_INTERVAL = 5000; // 5 seconds
const int MAX_NETWORKS = 50;

#define LED_PIN 2
#define BUTTON_PIN 0

// Web Server and DNS
WebServer server(WEB_PORT);
DNSServer dnsServer;
AsyncWebSocket ws("/ws");
Ticker scanTicker;
Ticker ledTicker;

// Data Structures
struct NetworkInfo {
  String ssid;
  String bssid;
  int8_t rssi;
  uint8_t channel;
  uint8_t encryption;
  bool hidden;
  unsigned long firstSeen;
  unsigned long lastSeen;
  int packetCount;
};

struct ScanResult {
  int networkCount;
  NetworkInfo networks[MAX_NETWORKS];
  unsigned long timestamp;
  bool scanning;
};

struct SystemStats {
  unsigned long uptime;
  int freeHeap;
  int maxAllocHeap;
  float wifiQuality;
  int totalScans;
  int networksDetected;
  int deauthPackets;
} systemStats;

struct AttackStats {
  bool deauthRunning;
  unsigned long attackStartTime;
  int targetIndex;
  unsigned long packetsSent;
  String targetSSID;
} attackStats;

// Global Variables
ScanResult lastScan;
bool isScanning = false;
bool ledState = false;
unsigned long lastUpdate = 0;
String macToHex(const uint8_t* mac) {
  String hex = "";
  for (int i = 0; i < 6; i++) {
    if (mac[i] < 0x10) hex += "0";
    hex += String(mac[i], HEX);
    if (i < 5) hex += ":";
  }
  hex.toUpperCase();
  return hex;
}

String getEncryptionType(uint8_t encryption) {
  switch (encryption) {
    case WIFI_AUTH_OPEN: return "Open";
    case WIFI_AUTH_WEP: return "WEP";
    case WIFI_AUTH_WPA_PSK: return "WPA";
    case WIFI_AUTH_WPA2_PSK: return "WPA2";
    case WIFI_AUTH_WPA_WPA2_PSK: return "WPA/WPA2";
    case WIFI_AUTH_WPA2_ENTERPRISE: return "WPA2 Enterprise";
    case WIFI_AUTH_WPA3_PSK: return "WPA3";
    case WIFI_AUTH_WPA2_WPA3_PSK: return "WPA2/WPA3";
    default: return "Unknown";
  }
}

String getSecurityLevel(uint8_t encryption, int rssi) {
  if (encryption == WIFI_AUTH_OPEN) return "NONE";
  if (rssi > -70) return "STRONG";
  if (rssi > -80) return "MEDIUM";
  return "WEAK";
}

// WiFi Scanning Function
void performWiFiScan() {
  if (isScanning) return;
  isScanning = true;
  
  Serial.println("Starting WiFi scan...");
  lastScan.scanning = true;
  
  int networksFound = WiFi.scanNetworks(/*async=*/false, /*hidden=*/true);
  
  lastScan.networkCount = min(networksFound, MAX_NETWORKS);
  lastScan.timestamp = millis();
  
  for (int i = 0; i < lastScan.networkCount; i++) {
    NetworkInfo& net = lastScan.networks[i];
    net.ssid = WiFi.SSID(i);
    net.bssid = macToHex(WiFi.BSSID(i));
    net.rssi = WiFi.RSSI(i);
    net.channel = WiFi.channel(i);
    net.encryption = WiFi.encryptionType(i);
    net.hidden = WiFi.isHidden(i);
    net.lastSeen = millis();
    
    // Find existing network or add new
    int existingIndex = -1;
    for (int j = 0; j < i; j++) {
      if (net.ssid == lastScan.networks[j].ssid && net.bssid == lastScan.networks[j].bssid) {
        existingIndex = j;
        break;
      }
    }
    
    if (existingIndex >= 0) {
      lastScan.networks[existingIndex].lastSeen = millis();
      lastScan.networks[existingIndex].packetCount++;
    }
    
    Serial.printf("Network %d: %s (%s) RSSI:%d Channel:%d Type:%s\n", 
                  i, net.ssid.c_str(), net.bssid.c_str(), net.rssi, 
                  net.channel, getEncryptionType(net.encryption).c_str());
  }
  
  systemStats.totalScans++;
  systemStats.networksDetected += networksFound;
  
  WiFi.scanDelete();
  lastScan.scanning = false;
  isScanning = false;
  
  Serial.printf("Scan completed. Found %d networks.\n", lastScan.networkCount);
}

// LED Control
void updateLED() {
  ledState = !ledState;
  digitalWrite(LED_PIN, ledState ? HIGH : LOW);
}

// Web Server Handlers
void handleRoot() {
  String html = R"HTML(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>bara.cpp - ESP32 WiFi Security Testing Tool</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body {
            background: #0a0a0a;
            color: #00ff00;
            font-family: 'Courier New', Monaco, Consolas, monospace;
            font-size: 14px;
            line-height: 1.4;
            overflow-x: hidden;
        }
        
        /* Matrix Background Effect */
        .matrix-bg {
            position: fixed;
            top: 0;
            left: 0;
            width: 100%;
            height: 100%;
            z-index: -1;
            background: radial-gradient(ellipse at center, #1a1a1a 0%, #0a0a0a 70%);
        }
        
        .matrix-bg::before {
            content: '';
            position: absolute;
            top: 0;
            left: 0;
            width: 100%;
            height: 100%;
            background: repeating-linear-gradient(
                0deg,
                transparent,
                transparent 2px,
                rgba(0, 255, 0, 0.1) 2px,
                rgba(0, 255, 0, 0.1) 4px
            );
            animation: matrix-rain 20s linear infinite;
        }
        
        @keyframes matrix-rain {
            0% { transform: translateY(-100vh); }
            100% { transform: translateY(100vh); }
        }
        
        /* Header */
        .header {
            background: linear-gradient(90deg, #ff0000 0%, #000000 50%, #ff0000 100%);
            padding: 15px;
            text-align: center;
            border-bottom: 2px solid #ff0000;
            box-shadow: 0 0 20px rgba(255, 0, 0, 0.5);
        }
        
        .title {
            font-size: 24px;
            font-weight: bold;
            text-shadow: 0 0 10px #ff0000;
            color: #ffffff;
            text-transform: uppercase;
            letter-spacing: 2px;
        }
        
        .subtitle {
            font-size: 12px;
            color: #cccccc;
            margin-top: 5px;
        }
        
        /* Container */
        .container {
            padding: 20px;
            max-width: 1400px;
            margin: 0 auto;
        }
        
        /* Dashboard Grid */
        .dashboard {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(300px, 1fr));
            gap: 20px;
            margin-bottom: 20px;
        }
        
        /* Cards */
        .card {
            background: linear-gradient(145deg, #1a1a1a 0%, #0f0f0f 100%);
            border: 1px solid #333;
            border-radius: 8px;
            padding: 20px;
            box-shadow: 0 4px 8px rgba(0, 0, 0, 0.5);
            transition: all 0.3s ease;
        }
        
        .card:hover {
            border-color: #ff0000;
            box-shadow: 0 0 15px rgba(255, 0, 0, 0.3);
        }
        
        .card-title {
            font-size: 16px;
            font-weight: bold;
            color: #ff0000;
            margin-bottom: 15px;
            text-transform: uppercase;
            border-bottom: 1px solid #333;
            padding-bottom: 5px;
        }
        
        /* Status Indicators */
        .status-indicator {
            display: inline-block;
            width: 12px;
            height: 12px;
            border-radius: 50%;
            margin-right: 8px;
            animation: pulse 2s infinite;
        }
        
        .status-online { background: #00ff00; }
        .status-scanning { background: #ff0000; }
        .status-attack { background: #ff4400; }
        
        @keyframes pulse {
            0%, 100% { opacity: 1; }
            50% { opacity: 0.3; }
        }
        
        /* Network Table */
        .network-table {
            width: 100%;
            border-collapse: collapse;
            font-size: 12px;
        }
        
        .network-table th {
            background: #1a1a1a;
            color: #ff0000;
            padding: 10px 8px;
            text-align: left;
            border-bottom: 1px solid #333;
            font-weight: bold;
        }
        
        .network-table td {
            padding: 8px;
            border-bottom: 1px solid #222;
        }
        
        .network-table tr:hover {
            background: rgba(255, 0, 0, 0.1);
        }
        
        /* Signal Strength Bars */
        .signal-bar {
            display: inline-block;
            width: 4px;
            height: 15px;
            margin: 1px;
            background: #333;
            border-radius: 1px;
        }
        
        .signal-bar.active {
            background: #00ff00;
            box-shadow: 0 0 5px #00ff00;
        }
        
        .signal-bar.medium {
            background: #ffaa00;
            box-shadow: 0 0 5px #ffaa00;
        }
        
        .signal-bar.weak {
            background: #ff0000;
            box-shadow: 0 0 5px #ff0000;
        }
        
        /* Buttons */
        .btn {
            background: linear-gradient(145deg, #333 0%, #1a1a1a 100%);
            border: 1px solid #555;
            color: #00ff00;
            padding: 10px 20px;
            border-radius: 4px;
            cursor: pointer;
            font-family: inherit;
            font-size: 14px;
            text-transform: uppercase;
            font-weight: bold;
            transition: all 0.3s ease;
            margin: 5px;
        }
        
        .btn:hover {
            border-color: #00ff00;
            box-shadow: 0 0 10px rgba(0, 255, 0, 0.5);
            transform: translateY(-1px);
        }
        
        .btn-danger {
            color: #ff0000;
            border-color: #ff0000;
        }
        
        .btn-danger:hover {
            border-color: #ff0000;
            box-shadow: 0 0 10px rgba(255, 0, 0, 0.5);
        }
        
        /* Security Badges */
        .security-badge {
            display: inline-block;
            padding: 2px 6px;
            border-radius: 3px;
            font-size: 10px;
            font-weight: bold;
            text-transform: uppercase;
        }
        
        .security-open { background: #ff0000; color: #ffffff; }
        .security-wep { background: #ff8800; color: #ffffff; }
        .security-wpa { background: #ffcc00; color: #000000; }
        .security-strong { background: #00ff00; color: #000000; }
        
        /* Charts */
        .chart-container {
            width: 100%;
            height: 200px;
            background: #0f0f0f;
            border: 1px solid #333;
            border-radius: 4px;
            position: relative;
            overflow: hidden;
        }
        
        .chart-bar {
            position: absolute;
            bottom: 0;
            width: 20px;
            background: linear-gradient(to top, #ff0000, #ff4444);
            border-radius: 2px 2px 0 0;
            transition: height 0.5s ease;
        }
        
        /* Terminal */
        .terminal {
            background: #0d0d0d;
            border: 1px solid #333;
            border-radius: 4px;
            height: 300px;
            overflow-y: auto;
            padding: 15px;
            font-family: 'Courier New', monospace;
            font-size: 12px;
            white-space: pre-wrap;
            color: #00ff00;
        }
        
        .terminal::-webkit-scrollbar {
            width: 8px;
        }
        
        .terminal::-webkit-scrollbar-track {
            background: #1a1a1a;
        }
        
        .terminal::-webkit-scrollbar-thumb {
            background: #333;
            border-radius: 4px;
        }
        
        .terminal::-webkit-scrollbar-thumb:hover {
            background: #555;
        }
        
        /* Glitch Effect */
        .glitch {
            animation: glitch 2s infinite;
        }
        
        @keyframes glitch {
            0%, 90%, 100% {
                text-shadow: 2px 0 #ff0000, -2px 0 #00ff00;
            }
            5% {
                text-shadow: -2px 0 #ff0000, 2px 0 #00ff00;
            }
            15% {
                text-shadow: 2px 0 #ff0000, -2px 0 #00ff00;
            }
            25% {
                text-shadow: -2px 0 #ff0000, 2px 0 #00ff00;
            }
        }
        
        /* Stats Display */
        .stats-grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(150px, 1fr));
            gap: 15px;
        }
        
        .stat-item {
            text-align: center;
            padding: 15px;
            background: linear-gradient(145deg, #1a1a1a 0%, #0f0f0f 100%);
            border: 1px solid #333;
            border-radius: 4px;
        }
        
        .stat-value {
            font-size: 24px;
            font-weight: bold;
            color: #00ff00;
            display: block;
        }
        
        .stat-label {
            font-size: 12px;
            color: #888;
            text-transform: uppercase;
        }
        
        /* Responsive */
        @media (max-width: 768px) {
            .dashboard {
                grid-template-columns: 1fr;
            }
            
            .network-table {
                font-size: 10px;
            }
            
            .network-table th,
            .network-table td {
                padding: 4px;
            }
        }
    </style>
</head>
<body>
    <div class="matrix-bg"></div>
    
    <div class="header">
        <div class="title glitch">bara.cpp - WiFi Security Testing Tool</div>
        <div class="subtitle">Created by: أحمد نور أحمد من قنا | Educational Security Analysis</div>
    </div>
    
    <div class="container">
        <!-- Status Dashboard -->
        <div class="dashboard">
            <div class="card">
                <div class="card-title">System Status</div>
                <div class="stats-grid">
                    <div class="stat-item">
                        <span class="status-indicator status-online" id="systemStatus"></span>
                        <span class="stat-value" id="uptime">00:00:00</span>
                        <span class="stat-label">Uptime</span>
                    </div>
                    <div class="stat-item">
                        <span class="stat-value" id="freeHeap">0</span>
                        <span class="stat-label">Free Heap</span>
                    </div>
                    <div class="stat-item">
                        <span class="stat-value" id="totalScans">0</span>
                        <span class="stat-label">Total Scans</span>
                    </div>
                    <div class="stat-item">
                        <span class="stat-value" id="networksDetected">0</span>
                        <span class="stat-label">Networks Found</span>
                    </div>
                </div>
            </div>
            
            <div class="card">
                <div class="card-title">Control Panel</div>
                <button class="btn" onclick="startScan()">Start Scan</button>
                <button class="btn" onclick="stopScan()">Stop Scan</button>
                <button class="btn" onclick="exportData()">Export Data</button>
                <button class="btn" onclick="clearLogs()">Clear Logs</button>
                <button class="btn btn-danger" onclick="simulateDeauth()" disabled>Deauth Attack (Demo)</button>
                <div style="margin-top: 10px; font-size: 12px; color: #888;">
                    Last scan: <span id="lastScanTime">Never</span>
                </div>
            </div>
        </div>
        
        <!-- Networks Found -->
        <div class="card" style="margin-bottom: 20px;">
            <div class="card-title">Detected Networks</div>
            <div id="networksList">
                <div style="text-align: center; color: #888; padding: 20px;">
                    No networks detected yet. Click "Start Scan" to begin.
                </div>
            </div>
        </div>
        
        <!-- Charts -->
        <div class="dashboard">
            <div class="card">
                <div class="card-title">Signal Strength Distribution</div>
                <div class="chart-container" id="signalChart"></div>
            </div>
            
            <div class="card">
                <div class="card-title">Security Types Distribution</div>
                <div class="chart-container" id="securityChart"></div>
            </div>
        </div>
        
        <!-- Terminal Output -->
        <div class="card" style="margin-top: 20px;">
            <div class="card-title">Terminal Output</div>
            <div class="terminal" id="terminalOutput">
                > bara.cpp initialized successfully
                > ESP32 WiFi Security Testing Tool ready
                > Waiting for scan command...
            </div>
        </div>
    </div>

    <script>
        let ws = null;
        let scanInterval = null;
        let systemUpdateInterval = null;
        
        function connectWebSocket() {
            const protocol = window.location.protocol === 'https:' ? 'wss:' : 'ws:';
            ws = new WebSocket(`${protocol}//${window.location.host}/ws`);
            
            ws.onopen = function() {
                logToTerminal('WebSocket connected. Real-time monitoring active.');
            };
            
            ws.onmessage = function(event) {
                const data = JSON.parse(event.data);
                handleWebSocketMessage(data);
            };
            
            ws.onclose = function() {
                logToTerminal('WebSocket disconnected. Attempting to reconnect...');
                setTimeout(connectWebSocket, 3000);
            };
            
            ws.onerror = function(error) {
                console.error('WebSocket error:', error);
            };
        }
        
        function handleWebSocketMessage(data) {
            if (data.type === 'scan_result') {
                updateNetworks(data.networks);
            } else if (data.type === 'stats') {
                updateSystemStats(data.stats);
            } else if (data.type === 'attack_status') {
                updateAttackStatus(data.status);
            } else if (data.type === 'log') {
                logToTerminal(data.message);
            }
        }
        
        function updateNetworks(networks) {
            const container = document.getElementById('networksList');
            
            if (networks.length === 0) {
                container.innerHTML = '<div style="text-align: center; color: #888; padding: 20px;">No networks detected</div>';
                return;
            }
            
            let html = `
                <table class="network-table">
                    <thead>
                        <tr>
                            <th>SSID</th>
                            <th>BSSID</th>
                            <th>Signal</th>
                            <th>Channel</th>
                            <th>Security</th>
                            <th>Quality</th>
                            <th>Actions</th>
                        </tr>
                    </thead>
                    <tbody>
            `;
            
            networks.forEach((network, index) => {
                const signalBars = generateSignalBars(network.rssi);
                const securityClass = getSecurityClass(network.encryption);
                const securityType = getSecurityType(network.encryption);
                const quality = getSignalQuality(network.rssi);
                
                html += `
                    <tr>
                        <td>${network.ssid || '[Hidden]'}</td>
                        <td style="font-size: 10px; color: #888;">${network.bssid}</td>
                        <td>${signalBars} (${network.rssi} dBm)</td>
                        <td>${network.channel}</td>
                        <td><span class="security-badge ${securityClass}">${securityType}</span></td>
                        <td>${quality}</td>
                        <td>
                            <button class="btn" style="padding: 4px 8px; font-size: 10px;" onclick="analyzeNetwork(${index})">
                                Analyze
                            </button>
                            ${network.encryption !== 0 ? `<button class="btn btn-danger" style="padding: 4px 8px; font-size: 10px;" onclick="targetNetwork(${index})">Target</button>` : ''}
                        </td>
                    </tr>
                `;
            });
            
            html += '</tbody></table>';
            container.innerHTML = html;
            
            updateCharts(networks);
            document.getElementById('lastScanTime').textContent = new Date().toLocaleTimeString();
        }
        
        function generateSignalBars(rssi) {
            const strength = Math.min(Math.max(Math.floor((rssi + 100) * 2), 0), 10);
            let html = '';
            
            for (let i = 0; i < 10; i++) {
                let barClass = 'signal-bar';
                if (i < strength) {
                    if (strength > 7) barClass += ' active';
                    else if (strength > 4) barClass += ' medium';
                    else barClass += ' weak';
                }
                html += `<span class="${barClass}"></span>`;
            }
            
            return html;
        }
        
        function getSecurityClass(encryption) {
            if (encryption === 0) return 'security-open';
            if (encryption === 1) return 'security-wep';
            if ([2, 3, 4, 5].includes(encryption)) return 'security-wpa';
            return 'security-wpa';
        }
        
        function getSecurityType(encryption) {
            if (encryption === 0) return 'Open';
            if (encryption === 1) return 'WEP';
            if (encryption === 2) return 'WPA';
            if (encryption === 3) return 'WPA2';
            if (encryption === 4) return 'WPA/WPA2';
            if (encryption === 5) return 'WPA2 Enterprise';
            if (encryption === 6) return 'WPA3';
            return 'Unknown';
        }
        
        function getSignalQuality(rssi) {
            if (rssi > -70) return 'Strong';
            if (rssi > -80) return 'Medium';
            return 'Weak';
        }
        
        function updateCharts(networks) {
            updateSignalChart(networks);
            updateSecurityChart(networks);
        }
        
        function updateSignalChart(networks) {
            const container = document.getElementById('signalChart');
            const ranges = [
                { label: '>-70', count: 0 },
                { label: '-70 to -80', count: 0 },
                { label: '<-80', count: 0 }
            ];
            
            networks.forEach(net => {
                if (net.rssi > -70) ranges[0].count++;
                else if (net.rssi > -80) ranges[1].count++;
                else ranges[2].count++;
            });
            
            let html = '';
            const maxCount = Math.max(...ranges.map(r => r.count));
            
            ranges.forEach((range, index) => {
                const height = maxCount > 0 ? (range.count / maxCount) * 150 : 0;
                const left = index * 60 + 10;
                html += `
                    <div class="chart-bar" style="left: ${left}px; height: ${height}px; background: linear-gradient(to top, ${getColorForRange(index)}, ${getColorForRange(index)}88);">
                    </div>
                    <div style="position: absolute; bottom: -20px; left: ${left}px; font-size: 10px; color: #888; width: 50px; text-align: center;">
                        ${range.label}<br>(${range.count})
                    </div>
                `;
            });
            
            container.innerHTML = html;
        }
        
        function updateSecurityChart(networks) {
            const container = document.getElementById('securityChart');
            const securityTypes = {};
            
            networks.forEach(net => {
                const type = getSecurityType(net.encryption);
                securityTypes[type] = (securityTypes[type] || 0) + 1;
            });
            
            const entries = Object.entries(securityTypes);
            let html = '';
            const maxCount = Math.max(...Object.values(securityTypes));
            
            entries.forEach(([type, count], index) => {
                const height = maxCount > 0 ? (count / maxCount) * 150 : 0;
                const left = index * 60 + 10;
                html += `
                    <div class="chart-bar" style="left: ${left}px; height: ${height}px; background: linear-gradient(to top, #00aa00, #44ff44);">
                    </div>
                    <div style="position: absolute; bottom: -20px; left: ${left}px; font-size: 10px; color: #888; width: 50px; text-align: center;">
                        ${type}<br>(${count})
                    </div>
                `;
            });
            
            container.innerHTML = html;
        }
        
        function getColorForRange(index) {
            const colors = ['#00ff00', '#ffaa00', '#ff0000'];
            return colors[index] || '#888888';
        }
        
        function updateSystemStats(stats) {
            document.getElementById('uptime').textContent = formatUptime(stats.uptime);
            document.getElementById('freeHeap').textContent = stats.freeHeap;
            document.getElementById('totalScans').textContent = stats.totalScans;
            document.getElementById('networksDetected').textContent = stats.networksDetected;
        }
        
        function formatUptime(millis) {
            const seconds = Math.floor(millis / 1000);
            const hours = Math.floor(seconds / 3600);
            const minutes = Math.floor((seconds % 3600) / 60);
            const secs = seconds % 60;
            return `${hours.toString().padStart(2, '0')}:${minutes.toString().padStart(2, '0')}:${secs.toString().padStart(2, '0')}`;
        }
        
        function logToTerminal(message) {
            const terminal = document.getElementById('terminalOutput');
            const timestamp = new Date().toLocaleTimeString();
            const logEntry = `[${timestamp}] ${message}`;
            
            terminal.innerHTML += `\n${logEntry}`;
            terminal.scrollTop = terminal.scrollHeight;
            
            // Keep only last 100 lines
            const lines = terminal.innerHTML.split('\n');
            if (lines.length > 100) {
                terminal.innerHTML = lines.slice(-100).join('\n');
            }
        }
        
        function startScan() {
            if (ws && ws.readyState === WebSocket.OPEN) {
                ws.send(JSON.stringify({ action: 'start_scan' }));
                logToTerminal('> Starting WiFi scan...');
                
                if (scanInterval) {
                    clearInterval(scanInterval);
                }
                scanInterval = setInterval(() => {
                    ws.send(JSON.stringify({ action: 'scan_now' }));
                }, 5000);
            }
        }
        
        function stopScan() {
            if (ws && ws.readyState === WebSocket.OPEN) {
                ws.send(JSON.stringify({ action: 'stop_scan' }));
                logToTerminal('> WiFi scan stopped.');
                
                if (scanInterval) {
                    clearInterval(scanInterval);
                    scanInterval = null;
                }
            }
        }
        
        function exportData() {
            if (ws && ws.readyState === WebSocket.OPEN) {
                ws.send(JSON.stringify({ action: 'export_data' }));
                logToTerminal('> Exporting scan data...');
            }
        }
        
        function clearLogs() {
            document.getElementById('terminalOutput').innerHTML = '> Terminal cleared\n> System monitoring active...';
        }
        
        function analyzeNetwork(index) {
            if (ws && ws.readyState === WebSocket.OPEN) {
                ws.send(JSON.stringify({ action: 'analyze_network', index: index }));
                logToTerminal(`> Analyzing network at index ${index}...`);
            }
        }
        
        function targetNetwork(index) {
            if (confirm('This is for educational purposes only. Are you sure you want to analyze this network?')) {
                if (ws && ws.readyState === WebSocket.OPEN) {
                    ws.send(JSON.stringify({ action: 'target_network', index: index }));
                    logToTerminal(`> Targeting network at index ${index} for analysis...`);
                }
            }
        }
        
        function simulateDeauth() {
            if (confirm('WARNING: This is a simulation for educational purposes only. Do not proceed with any illegal activities.')) {
                if (ws && ws.readyState === WebSocket.OPEN) {
                    ws.send(JSON.stringify({ action: 'simulate_deauth' }));
                    logToTerminal('> Deauthentication attack simulation started...');
                }
            }
        }
        
        // System updates
        setInterval(() => {
            if (ws && ws.readyState === WebSocket.OPEN) {
                ws.send(JSON.stringify({ action: 'get_stats' }));
            }
        }, 10000);
        
        // Initialize
        document.addEventListener('DOMContentLoaded', function() {
            connectWebSocket();
            logToTerminal('> bara.cpp interface loaded');
            logToTerminal('> Real-time monitoring initialized');
        });
    </script>
</body>
</html>
)HTML";

  server.send(200, "text/html", html);
}

void handleAPI() {
  if (server.hasArg("action")) {
    String action = server.arg("action");
    
    if (action == "scan") {
      performWiFiScan();
      server.send(200, "application/json", "{\"status\":\"success\",\"message\":\"Scan completed\"}");
    } else if (action == "stats") {
      DynamicJsonDocument doc(1024);
      doc["uptime"] = systemStats.uptime;
      doc["freeHeap"] = systemStats.freeHeap;
      doc["maxAllocHeap"] = systemStats.maxAllocHeap;
      doc["wifiQuality"] = systemStats.wifiQuality;
      doc["totalScans"] = systemStats.totalScans;
      doc["networksDetected"] = systemStats.networksDetected;
      
      String response;
      serializeJson(doc, response);
      server.send(200, "application/json", response);
    } else if (action == "networks") {
      DynamicJsonDocument doc(4096);
      JsonArray networks = doc.to<JsonArray>();
      
      for (int i = 0; i < lastScan.networkCount; i++) {
        JsonObject network = networks.createNestedObject();
        network["ssid"] = lastScan.networks[i].ssid;
        network["bssid"] = lastScan.networks[i].bssid;
        network["rssi"] = lastScan.networks[i].rssi;
        network["channel"] = lastScan.networks[i].channel;
        network["encryption"] = lastScan.networks[i].encryption;
        network["hidden"] = lastScan.networks[i].hidden;
      }
      
      String response;
      serializeJson(doc, response);
      server.send(200, "application/json", response);
    } else if (action == "export") {
      String csv = "SSID,BSSID,RSSI,Channel,Encryption,Security Level\n";
      for (int i = 0; i < lastScan.networkCount; i++) {
        csv += lastScan.networks[i].ssid + ",";
        csv += lastScan.networks[i].bssid + ",";
        csv += String(lastScan.networks[i].rssi) + ",";
        csv += String(lastScan.networks[i].channel) + ",";
        csv += getEncryptionType(lastScan.networks[i].encryption) + ",";
        csv += getSecurityLevel(lastScan.networks[i].encryption, lastScan.networks[i].rssi) + "\n";
      }
      
      server.sendHeader("Content-Disposition", "attachment; filename=scan_results.csv");
      server.send(200, "text/csv", csv);
    }
  } else {
    server.send(400, "text/plain", "Bad Request");
  }
}

// WebSocket Handlers
void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      // Send initial data
      sendCurrentData(client);
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      break;
    case WS_EVT_DATA:
      handleWsMessage(client, arg, data, len);
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
  }
}

void handleWsMessage(AsyncWebSocketClient *client, void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;
    String message = (char*)data;
    
    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, message);
    
    if (error) {
      Serial.println("JSON parse error");
      return;
    }
    
    String action = doc["action"];
    
    if (action == "start_scan") {
      if (!isScanning) {
        scanTicker.attach(SCAN_INTERVAL / 1000.0, performWiFiScan);
        Serial.println("Auto-scan started");
      }
    } else if (action == "stop_scan") {
      scanTicker.detach();
      Serial.println("Auto-scan stopped");
    } else if (action == "scan_now") {
      performWiFiScan();
      sendScanResult();
    } else if (action == "get_stats") {
      updateSystemStats();
      sendStats();
    } else if (action == "analyze_network") {
      int index = doc["index"];
      analyzeNetwork(index, client->id());
    } else if (action == "target_network") {
      int index = doc["index"];
      targetNetwork(index, client->id());
    } else if (action == "simulate_deauth") {
      simulateDeauthAttack(client->id());
    } else if (action == "export_data") {
      exportScanData(client->id());
    }
  }
}

void sendScanResult() {
  DynamicJsonDocument doc(4096);
  JsonArray networks = doc.createNestedArray("networks");
  
  for (int i = 0; i < lastScan.networkCount; i++) {
    JsonObject network = networks.createNestedObject();
    network["ssid"] = lastScan.networks[i].ssid;
    network["bssid"] = lastScan.networks[i].bssid;
    network["rssi"] = lastScan.networks[i].rssi;
    network["channel"] = lastScan.networks[i].channel;
    network["encryption"] = lastScan.networks[i].encryption;
    network["hidden"] = lastScan.networks[i].hidden;
    network["securityLevel"] = getSecurityLevel(lastScan.networks[i].encryption, lastScan.networks[i].rssi);
  }
  
  String response;
  serializeJson(doc, response);
  
  ws.textAll("{\"type\":\"scan_result\",\"networks\":" + response + "}");
}

void sendStats() {
  DynamicJsonDocument doc(1024);
  doc["uptime"] = systemStats.uptime;
  doc["freeHeap"] = systemStats.freeHeap;
  doc["maxAllocHeap"] = systemStats.maxAllocHeap;
  doc["wifiQuality"] = systemStats.wifiQuality;
  doc["totalScans"] = systemStats.totalScans;
  doc["networksDetected"] = systemStats.networksDetected;
  doc["deauthPackets"] = systemStats.deauthPackets;
  
  String response;
  serializeJson(doc, response);
  
  ws.textAll("{\"type\":\"stats\",\"stats\":" + response + "}");
}

void sendCurrentData(AsyncWebSocketClient *client) {
  // Send current scan results
  sendScanResult();
  
  // Send current stats
  updateSystemStats();
  sendStats();
}

void updateSystemStats() {
  systemStats.uptime = millis();
  systemStats.freeHeap = ESP.getFreeHeap();
  systemStats.maxAllocHeap = ESP.getMaxAllocHeap();
  
  // Calculate WiFi quality based on connected status
  if (WiFi.status() == WL_CONNECTED) {
    long rssi = WiFi.RSSI();
    systemStats.wifiQuality = constrain(map(rssi, -100, -50, 0, 100), 0, 100);
  } else {
    systemStats.wifiQuality = 0;
  }
}

void analyzeNetwork(int index, uint32_t clientId) {
  if (index >= 0 && index < lastScan.networkCount) {
    NetworkInfo& net = lastScan.networks[index];
    
    Serial.printf("Analyzing network: %s (%s)\n", net.ssid.c_str(), net.bssid.c_str());
    
    DynamicJsonDocument doc(1024);
    doc["ssid"] = net.ssid;
    doc["bssid"] = net.bssid;
    doc["encryptionType"] = getEncryptionType(net.encryption);
    doc["securityLevel"] = getSecurityLevel(net.encryption, net.rssi);
    doc["channel"] = net.channel;
    doc["signalStrength"] = net.rssi;
    doc["recommendations"] = "Network security analysis complete";
    
    String response;
    serializeJson(doc, response);
    
    ws.text(clientId, "{\"type\":\"analysis\",\"result\":" + response + "}");
  }
}

void targetNetwork(int index, uint32_t clientId) {
  if (index >= 0 && index < lastScan.networkCount) {
    NetworkInfo& net = lastScan.networks[index];
    attackStats.targetIndex = index;
    attackStats.targetSSID = net.ssid;
    
    Serial.printf("Targeting network: %s (%s)\n", net.ssid.c_str(), net.bssid.c_str());
    
    ws.text(clientId, "{\"type\":\"attack_status\",\"status\":\"targeting\",\"target\":\"" + net.ssid + "\"}");
  }
}

void simulateDeauthAttack(uint32_t clientId) {
  Serial.println("Deauthentication attack simulation started");
  
  attackStats.deauthRunning = true;
  attackStats.attackStartTime = millis();
  attackStats.packetsSent = 0;
  
  // Simulate sending deauth packets
  for (int i = 0; i < 10; i++) {
    delay(500);
    attackStats.packetsSent += 10;
    systemStats.deauthPackets += 10;
    
    DynamicJsonDocument doc(256);
    doc["packetsSent"] = attackStats.packetsSent;
    doc["elapsedTime"] = millis() - attackStats.attackStartTime;
    
    String response;
    serializeJson(doc, response);
    
    ws.text(clientId, "{\"type\":\"attack_progress\",\"data\":" + response + "}");
  }
  
  attackStats.deauthRunning = false;
  
  ws.text(clientId, "{\"type\":\"attack_complete\",\"message\":\"Simulation completed. Remember: This was for educational purposes only!\"}");
}

void exportScanData(uint32_t clientId) {
  String csvData = "SSID,BSSID,RSSI,Channel,Encryption,Security Level,First Seen,Last Seen\n";
  
  for (int i = 0; i < lastScan.networkCount; i++) {
    NetworkInfo& net = lastScan.networks[i];
    csvData += "\"" + net.ssid + "\",";
    csvData += net.bssid + ",";
    csvData += String(net.rssi) + ",";
    csvData += String(net.channel) + ",";
    csvData += "\"" + getEncryptionType(net.encryption) + "\",";
    csvData += "\"" + getSecurityLevel(net.encryption, net.rssi) + "\",";
    csvData += String(net.firstSeen) + ",";
    csvData += String(net.lastSeen) + "\n";
  }
  
  ws.text(clientId, "{\"type\":\"export_data\",\"csv\":\"" + csvData + "\"}");
}

// Setup Function
void setup() {
  Serial.begin(115200);
  Serial.println("Initializing bara.cpp - ESP32 WiFi Security Testing Tool");
  
  // Initialize GPIO
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  
  // Initialize SPIFFS
  if (!SPIFFS.begin()) {
    Serial.println("SPIFFS Mount Failed");
    return;
  }
  
  // Initialize WiFi Access Point
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_NAME, AP_PASSWORD);
  
  IPAddress IP = WiFi.softAPIP();
  Serial.print("Access Point IP: ");
  Serial.println(IP);
  
  // Start DNS Server for captive portal
  dnsServer.start(53, "*", IP);
  
  // Initialize WebSocket
  ws.onEvent(onWsEvent);
  server.addHandler(&ws);
  
  // Setup server routes
  server.on("/", handleRoot);
  server.on("/api", handleAPI);
  server.onNotFound([]() {
    server.send(404, "text/plain", "Not Found");
  });
  
  // Start web server
  server.begin();
  Serial.println("Web server started on port " + String(WEB_PORT));
  
  // Initialize stats
  memset(&systemStats, 0, sizeof(systemStats));
  memset(&attackStats, 0, sizeof(attackStats));
  memset(&lastScan, 0, sizeof(lastScan));
  
  // Start LED blink
  ledTicker.attach(1.0, updateLED);
  
  // Initial scan
  delay(1000);
  performWiFiScan();
  
  Serial.println("bara.cpp initialized successfully!");
  Serial.println("Developer: أحمد نور أحمد من قنا");
  Serial.println("Tool: WiFi Security Testing (Educational Only)");
}

// Main Loop
void loop() {
  // Handle DNS requests
  dnsServer.processNextRequest();
  
  // Handle WebSocket clients
  ws.cleanupClients();
  
  // Handle web server
  server.handleClient();
  
  // Update system stats
  if (millis() - lastUpdate > 10000) {
    updateSystemStats();
    lastUpdate = millis();
  }
  
  // Handle button press for manual scan
  if (digitalRead(BUTTON_PIN) == LOW) {
    delay(50); // Debounce
    if (digitalRead(BUTTON_PIN) == LOW) {
      Serial.println("Manual scan triggered by button");
      performWiFiScan();
      delay(1000);
    }
  }
  
  // Handle automatic scan
  if (!isScanning && scanTicker.active()) {
    // Scan is handled by ticker
  }
  
  yield();
}