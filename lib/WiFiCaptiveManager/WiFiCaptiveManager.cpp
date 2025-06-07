#include "WiFiCaptiveManager.h"
#include <EEPROM.h>
#include <WiFi.h>

// EEPROM Configuration
#define EEPROM_SIZE 512
#define SSID_ADDR 0
#define SSID_MAX_LEN 32
#define PASSWORD_ADDR 64
#define PASSWORD_MAX_LEN 64
#define MAGIC_ADDR 128
#define MAGIC_VALUE 0xABCD

WiFiCaptiveManager::WiFiCaptiveManager(AsyncWebServer& srv, DNSServer& dns)
    : server(srv), dnsServer(dns), captivePortalActive(false) {}

void WiFiCaptiveManager::begin() {
    Serial.println("[WiFiCM] Initializing WiFi Captive Manager");
    
    if (!EEPROM.begin(EEPROM_SIZE)) {
        Serial.println("[WiFiCM] EEPROM initialization failed");
        return;
    }
    
    loadCredentials();
    connectToWiFi();
    
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[WiFiCM] Starting captive portal");
        startCaptivePortal();
    }
}

void WiFiCaptiveManager::loop() {
    if (captivePortalActive) {
        dnsServer.processNextRequest();
        
        // Optional: Add periodic status check
        static unsigned long lastCheck = 0;
        if (millis() - lastCheck > 30000) { // Every 30 seconds
            Serial.printf("[WiFiCM] Captive portal active, connected clients: %d\n", 
                         WiFi.softAPgetStationNum());
            lastCheck = millis();
        }
    }
}

void WiFiCaptiveManager::loadCredentials() {
    // Check if EEPROM has valid data
    uint16_t magic = (EEPROM.read(MAGIC_ADDR) << 8) | EEPROM.read(MAGIC_ADDR + 1);
    
    if (magic != MAGIC_VALUE) {
        // First time or corrupted data - clear credentials
        ssid = "";
        password = "";
        Serial.println("[WiFiCM] No valid credentials found in EEPROM");
        return;
    }
    
    // Read SSID
    char ssidBuffer[SSID_MAX_LEN + 1] = {0};
    for (int i = 0; i < SSID_MAX_LEN; i++) {
        ssidBuffer[i] = EEPROM.read(SSID_ADDR + i);
        if (ssidBuffer[i] == 0) break;
    }
    ssid = String(ssidBuffer);
    
    // Read Password
    char passwordBuffer[PASSWORD_MAX_LEN + 1] = {0};
    for (int i = 0; i < PASSWORD_MAX_LEN; i++) {
        passwordBuffer[i] = EEPROM.read(PASSWORD_ADDR + i);
        if (passwordBuffer[i] == 0) break;
    }
    password = String(passwordBuffer);
    
    Serial.printf("[WiFiCM] Loaded credentials - SSID: %s\n", 
                  ssid.isEmpty() ? "(empty)" : ssid.c_str());
}
void WiFiCaptiveManager::saveCredentials(const String& newSSID, const String& newPassword) {
    Serial.printf("[WiFiCM] saveCredentials called - SSID: '%s', Password length: %d\n", 
                 newSSID.c_str(), newPassword.length());
    
    // Clear SSID area first
    Serial.println("[WiFiCM] Clearing SSID area in EEPROM...");
    for (int i = 0; i < SSID_MAX_LEN; i++) {
        EEPROM.write(SSID_ADDR + i, 0);
    }
    
    // Write SSID
    Serial.printf("[WiFiCM] Writing SSID to EEPROM (length: %d)...\n", newSSID.length());
    int ssidLen = min(newSSID.length(), (size_t)SSID_MAX_LEN - 1);
    for (int i = 0; i < ssidLen; i++) {
        EEPROM.write(SSID_ADDR + i, newSSID[i]);
    }
    
    // Clear Password area
    Serial.println("[WiFiCM] Clearing password area in EEPROM...");
    for (int i = 0; i < PASSWORD_MAX_LEN; i++) {
        EEPROM.write(PASSWORD_ADDR + i, 0);
    }
    
    // Write Password
    Serial.printf("[WiFiCM] Writing password to EEPROM (length: %d)...\n", newPassword.length());
    int passwordLen = min(newPassword.length(), (size_t)PASSWORD_MAX_LEN - 1);
    for (int i = 0; i < passwordLen; i++) {
        EEPROM.write(PASSWORD_ADDR + i, newPassword[i]);
    }
    
    // Write magic value to indicate valid data
    Serial.println("[WiFiCM] Writing magic value to EEPROM...");
    EEPROM.write(MAGIC_ADDR, 0xAB);
    EEPROM.write(MAGIC_ADDR + 1, 0xCD);
    
    // Commit changes to EEPROM
    Serial.println("[WiFiCM] Committing EEPROM changes...");
    if (EEPROM.commit()) {
        Serial.println("[WiFiCM] ✅ EEPROM commit successful");
        
        // Verify what was written
        Serial.println("[WiFiCM] Verifying written data...");
        
        // Read back SSID
        char readSSID[SSID_MAX_LEN + 1] = {0};
        for (int i = 0; i < ssidLen; i++) {
            readSSID[i] = EEPROM.read(SSID_ADDR + i);
        }
        Serial.printf("[WiFiCM] Verified SSID: '%s'\n", readSSID);
        
        // Read back magic
        uint8_t magic1 = EEPROM.read(MAGIC_ADDR);
        uint8_t magic2 = EEPROM.read(MAGIC_ADDR + 1);
        Serial.printf("[WiFiCM] Verified magic: 0x%02X 0x%02X\n", magic1, magic2);
        
    } else {
        Serial.println("[WiFiCM] ❌ EEPROM commit failed!");
    }
}
void WiFiCaptiveManager::clearCredentials() {
    Serial.println("[WiFiCM] Clearing saved credentials");
    
    // Clear magic value
    EEPROM.write(MAGIC_ADDR, 0);
    EEPROM.write(MAGIC_ADDR + 1, 0);
    
    // Clear SSID and Password areas
    for (int i = 0; i < SSID_MAX_LEN; i++) {
        EEPROM.write(SSID_ADDR + i, 0);
    }
    for (int i = 0; i < PASSWORD_MAX_LEN; i++) {
        EEPROM.write(PASSWORD_ADDR + i, 0);
    }
    
    EEPROM.commit();
    
    ssid = "";
    password = "";
}

void WiFiCaptiveManager::connectToWiFi() {
    Serial.println("[WiFiCM] Attempting WiFi connection");
    
    if (ssid.isEmpty()) {
        Serial.println("[WiFiCM] No SSID available");
        return;
    }
    
    Serial.println("[WiFiCM] SSID is not empty");
    Serial.printf("[WiFiCM] Connecting to: %s\n", ssid.c_str());
    Serial.printf("[WiFiCM] Password: %s\n", password.c_str());
    
    WiFi.begin(ssid.c_str(), password.c_str());
    unsigned long start = millis();
    
    while (WiFi.status() != WL_CONNECTED && millis() - start < 10000) {
        delay(500);
        Serial.print(".");
    }
    Serial.println();
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("[WiFiCM] Connected! IP: %s\n", WiFi.localIP().toString().c_str());
    } else {
        Serial.printf("[WiFiCM] Connection failed. Status: %d\n", WiFi.status());
    }
}

void WiFiCaptiveManager::startCaptivePortal() {
    startAccessPoint();
    setupWebServer();
    
    // Get the AP IP address
    IPAddress apIP = WiFi.softAPIP();
    
    // Configure DNS server settings
    dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
    dnsServer.setTTL(300);
    
    // Start DNS server with explicit parameters
    // Parameters: port, domain_wildcard, redirect_IP
    bool dnsStarted = dnsServer.start(53, "*", apIP);
    
    if (!dnsStarted) {
        Serial.println("[WiFiCM] Failed to start DNS server");
        // Try with different port if 53 fails
        dnsStarted = dnsServer.start(5353, "*", apIP);
        if (dnsStarted) {
            Serial.println("[WiFiCM] DNS started on alternative port 5353");
        }
    } else {
        Serial.println("[WiFiCM] DNS server started successfully on port 53");
    }
    
    captivePortalActive = dnsStarted;
    
    if (captivePortalActive) {
        Serial.printf("[WiFiCM] Captive portal active - AP IP: %s\n", apIP.toString().c_str());
    }
}


void WiFiCaptiveManager::startAccessPoint() {
    WiFi.mode(WIFI_AP_STA);
    
    String apName = "ESP32_Config_" + String((uint32_t)ESP.getEfuseMac(), HEX);
    apName.toUpperCase();
    
    IPAddress local_IP(192, 168, 4, 1);
    IPAddress gateway(192, 168, 4, 1);
    IPAddress subnet(255, 255, 255, 0);
    
    WiFi.softAPConfig(local_IP, gateway, subnet);
    
    // Try different configurations if first fails
    bool apStarted = WiFi.softAP(apName.c_str(), nullptr, 6, 0, 4);  // Channel 6
    if (!apStarted) {
        apStarted = WiFi.softAP(apName.c_str(), nullptr, 1, 0, 4);   // Channel 1
    }
    
    if (!apStarted) {
        Serial.println("[WiFiCM] Failed to start Access Point");
        return;
    }
    
    delay(2000);  // Longer delay for AP to stabilize
    
    Serial.printf("[WiFiCM] Access Point '%s' started\n", apName.c_str());
    Serial.printf("[WiFiCM] AP IP: %s\n", WiFi.softAPIP().toString().c_str());
}

void WiFiCaptiveManager::sendCaptivePortalResponse(AsyncWebServerRequest *request) {
    AsyncWebServerResponse *response = request->beginResponse(200, "text/html", getCaptivePortalHTML());
    
    // Add headers to ensure proper captive portal behavior
    response->addHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    response->addHeader("Pragma", "no-cache");
    response->addHeader("Expires", "0");
    
    request->send(response);
}


void WiFiCaptiveManager::setupWebServer() {
    // CORS headers for all responses
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "Content-Type");
    
    // Root route serves the captive portal
    server.on("/", HTTP_GET, [this](AsyncWebServerRequest *request) {
        Serial.println("[WiFiCM] Root request received");
        sendCaptivePortalResponse(request);
    });
    
    // API Routes
    server.on("/scan", HTTP_GET, [this](AsyncWebServerRequest *request) {
        Serial.println("[WiFiCM] Scan request received");
        handleScanNetworks(request);
    });
    
    // **CRITICAL FIX: Register the /connect endpoint**
    server.on("/connect", HTTP_POST, 
        [](AsyncWebServerRequest *request){
            // This handles URL-encoded form data (if any)
            Serial.println("[WiFiCM] Connect POST request received (URL-encoded handler)");
        },
        [](AsyncWebServerRequest *request, const String& filename, size_t index, uint8_t *data, size_t len, bool final){
            // This handles file uploads (not used here)
        },
        [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            // This handles JSON body data
            Serial.println("[WiFiCM] Connect POST request received (JSON body handler)");
            handleConnect(request, data, len, index, total);
        }
    );
    
    server.on("/reset", HTTP_POST, [this](AsyncWebServerRequest *request) {
        Serial.println("[WiFiCM] Reset request received");
        clearCredentials();
        request->send(200, "application/json", "{\"success\":true,\"message\":\"Credentials cleared\"}");
        delay(1000);
        ESP.restart();
    });
    
    // Handle OPTIONS for CORS preflight
    server.on("/connect", HTTP_OPTIONS, [](AsyncWebServerRequest *request) {
        Serial.println("[WiFiCM] Connect OPTIONS request received");
        request->send(200);
    });
    
    // Captive portal detection URLs
    const char* captiveUrls[] = {
        "/generate_204",
        "/gen_204", 
        "/hotspot-detect.html",
        "/library/test/success.html",
        "/connecttest.txt",
        "/ncsi.txt",
        "/success.txt"
    };
    
    // Set up all detection URLs to serve the captive portal
    for (const char* url : captiveUrls) {
        server.on(url, HTTP_GET, [this](AsyncWebServerRequest *request) {
            Serial.printf("[WiFiCM] Captive detection URL requested: %s\n", request->url().c_str());
            sendCaptivePortalResponse(request);
        });
    }
    
    // Special redirect endpoints
    server.on("/fwlink", HTTP_GET, [this](AsyncWebServerRequest *request) {
        Serial.println("[WiFiCM] FWLink redirect requested");
        request->redirect("http://" + WiFi.softAPIP().toString());
    });
    
    server.on("/redirect", HTTP_GET, [this](AsyncWebServerRequest *request) {
        Serial.println("[WiFiCM] Redirect requested");
        request->redirect("http://" + WiFi.softAPIP().toString());
    });
    
    // Catch-all for captive portal - this should be LAST
    server.onNotFound([this](AsyncWebServerRequest *request) {
        Serial.printf("[WiFiCM] Not found request for: %s\n", request->url().c_str());
        sendCaptivePortalResponse(request);
    });
    
    server.begin();
    Serial.println("[WiFiCM] Web server started with all routes registered");
}

void WiFiCaptiveManager::handleScanNetworks(AsyncWebServerRequest *request) {
    Serial.println("[WiFiCM] Scanning networks...");
    
    WiFi.mode(WIFI_AP_STA);  // Ensure STA is enabled
    int n = WiFi.scanNetworks(true, true);  // Async scan
    delay(1000);
    
    DynamicJsonDocument doc(1024);
    JsonArray arr = doc.to<JsonArray>();

    for (int i = 0; i < n; ++i) {
        String networkSSID = WiFi.SSID(i);
        if (networkSSID.length() > 0) {  // Skip hidden networks
            JsonObject net = arr.createNestedObject();
            net["ssid"] = networkSSID;
            net["rssi"] = WiFi.RSSI(i);
        }
    }

    String json;
    serializeJson(doc, json);
    request->send(200, "application/json", json);
    
    Serial.printf("[WiFiCM] Found %d networks\n", n);
}

void WiFiCaptiveManager::handleConnect(AsyncWebServerRequest *request, uint8_t *data, 
                                     size_t len, size_t index, size_t total) {
    Serial.printf("[WiFiCM] handleConnect called - index: %zu, len: %zu, total: %zu\n", index, len, total);
    
    static String jsonBuffer = "";  // Static to persist across calls
    
    // Accumulate data chunks
    for (size_t i = 0; i < len; i++) {
        jsonBuffer += (char)data[i];
    }
    
    // Check if we have all the data
    if (index + len >= total) {
        Serial.printf("[WiFiCM] Complete JSON received: %s\n", jsonBuffer.c_str());
        
        DynamicJsonDocument doc(512);
        DeserializationError error = deserializeJson(doc, jsonBuffer);

        if (error) {
            Serial.printf("[WiFiCM] JSON parse error: %s\n", error.c_str());
            request->send(400, "application/json", "{\"success\":false,\"message\":\"Invalid JSON\"}");
            jsonBuffer = "";  // Clear buffer
            return;
        }

        String newSSID = doc["ssid"].as<String>();
        String newPassword = doc["password"].as<String>();
        
        Serial.printf("[WiFiCM] Parsed credentials - SSID: '%s', Password length: %d\n", 
                     newSSID.c_str(), newPassword.length());

        // Validate input
        if (newSSID.isEmpty()) {
            Serial.println("[WiFiCM] Empty SSID provided");
            request->send(400, "application/json", "{\"success\":false,\"message\":\"SSID cannot be empty\"}");
            jsonBuffer = "";
            return;
        }
        
        if (newSSID.length() >= SSID_MAX_LEN) {
            Serial.printf("[WiFiCM] SSID too long: %d chars\n", newSSID.length());
            request->send(400, "application/json", "{\"success\":false,\"message\":\"SSID too long\"}");
            jsonBuffer = "";
            return;
        }
        
        if (newPassword.length() >= PASSWORD_MAX_LEN) {
            Serial.printf("[WiFiCM] Password too long: %d chars\n", newPassword.length());
            request->send(400, "application/json", "{\"success\":false,\"message\":\"Password too long\"}");
            jsonBuffer = "";
            return;
        }

        Serial.println("[WiFiCM] Validation passed, saving credentials...");

        // Save credentials
        saveCredentials(newSSID, newPassword);
        
        // Update current credentials
        ssid = newSSID;
        password = newPassword;

        Serial.println("[WiFiCM] Credentials saved successfully");
        request->send(200, "application/json", "{\"success\":true,\"message\":\"Credentials saved, restarting...\"}");
        
        // Clear buffer
        jsonBuffer = "";
        
        Serial.println("[WiFiCM] Restarting ESP32 in 2 seconds...");
        delay(2000);
        ESP.restart();
    } else {
        Serial.printf("[WiFiCM] Partial data received, waiting for more... (buffer size: %d)\n", jsonBuffer.length());
    }
}

const char* WiFiCaptiveManager::getCaptivePortalHTML() {
    return R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ESP32 WiFi Setup</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 0; padding: 20px; background: #f0f0f0; }
        .container { max-width: 400px; margin: 0 auto; background: white; padding: 30px; border-radius: 10px; box-shadow: 0 4px 6px rgba(0,0,0,0.1); }
        h1 { color: #333; text-align: center; margin-bottom: 30px; }
        .form-group { margin-bottom: 20px; }
        label { display: block; margin-bottom: 5px; color: #555; font-weight: bold; }
        input[type="text"], input[type="password"], select { 
            width: 100%; padding: 12px; border: 2px solid #ddd; border-radius: 5px; 
            font-size: 16px; box-sizing: border-box;
        }
        input:focus { border-color: #4CAF50; outline: none; }
        button { 
            width: 100%; padding: 12px; background: #4CAF50; color: white; 
            border: none; border-radius: 5px; font-size: 16px; cursor: pointer; 
            transition: background 0.3s; margin-bottom: 10px;
        }
        button:hover { background: #45a049; }
        .reset-btn { background: #f44336; }
        .reset-btn:hover { background: #da190b; }
        .status { margin-top: 15px; padding: 10px; border-radius: 5px; text-align: center; }
        .success { background: #d4edda; color: #155724; border: 1px solid #c3e6cb; }
        .error { background: #f8d7da; color: #721c24; border: 1px solid #f5c6cb; }
        .network-list { margin-bottom: 20px; }
        .network-item { 
            padding: 10px; margin: 5px 0; background: #f8f9fa; 
            border-radius: 5px; cursor: pointer; border: 1px solid #dee2e6;
        }
        .network-item:hover { background: #e9ecef; }
        .network-item.selected { background: #4CAF50; color: white; }
        .signal-strength { float: right; font-weight: bold; }
    </style>
</head>
<body>
    <div class="container">
        <h1>🔧 ESP32 WiFi Setup</h1>
        
        <div class="network-list" id="networkList">
            <h3>Available Networks:</h3>
            <div id="networks">Scanning...</div>
            <button type="button" onclick="scanNetworks()">🔄 Refresh Networks</button>
        </div>

        <form id="wifiForm">
            <div class="form-group">
                <label for="ssid">Network Name (SSID):</label>
                <input type="text" id="ssid" name="ssid" required maxlength="31">
            </div>
            
            <div class="form-group">
                <label for="password">Password:</label>
                <input type="text" id="password" name="password" maxlength="63">
            </div>
            
            <button type="submit">💾 Save & Connect</button>
            <button type="button" class="reset-btn" onclick="resetCredentials()">🗑️ Clear Saved WiFi</button>
        </form>
        
        <div id="status"></div>
    </div>

    <script>
        let selectedNetwork = '';

        function scanNetworks() {
            document.getElementById('networks').innerHTML = 'Scanning...';
            fetch('/scan')
                .then(response => response.json())
                .then(data => displayNetworks(data))
                .catch(error => {
                    document.getElementById('networks').innerHTML = 'Scan failed';
                    console.error('Error:', error);
                });
        }

        function displayNetworks(networks) {
            const container = document.getElementById('networks');
            if (networks.length === 0) {
                container.innerHTML = 'No networks found';
                return;
            }

            container.innerHTML = networks.map(network => 
                `<div class="network-item" onclick="selectNetwork('${network.ssid}')">
                    ${network.ssid}
                    <span class="signal-strength">${getSignalIcon(network.rssi)}</span>
                </div>`
            ).join('');
        }

        function selectNetwork(ssid) {
            selectedNetwork = ssid;
            document.getElementById('ssid').value = ssid;
            
            // Visual feedback
            document.querySelectorAll('.network-item').forEach(item => {
                item.classList.remove('selected');
            });
            event.target.classList.add('selected');
        }

        function getSignalIcon(rssi) {
            if (rssi > -50) return '📶📶📶📶';
            if (rssi > -60) return '📶📶📶';
            if (rssi > -70) return '📶📶';
            return '📶';
        }

        function resetCredentials() {
            if (confirm('Are you sure you want to clear saved WiFi credentials?')) {
                showStatus('Clearing credentials...', 'info');
                
                fetch('/reset', {
                    method: 'POST'
                })
                .then(response => response.json())
                .then(data => {
                    if (data.success) {
                        showStatus('✅ Credentials cleared! Device will restart...', 'success');
                        setTimeout(() => window.location.reload(), 3000);
                    } else {
                        showStatus('❌ Failed to clear credentials', 'error');
                    }
                })
                .catch(error => {
                    showStatus('❌ Request failed', 'error');
                    console.error('Error:', error);
                });
            }
        }

        document.getElementById('wifiForm').addEventListener('submit', function(e) {
            e.preventDefault();
            
            const ssid = document.getElementById('ssid').value;
            const password = document.getElementById('password').value;
            
            if (!ssid) {
                showStatus('Please enter a network name', 'error');
                return;
            }
            
            showStatus('Connecting to ' + ssid + '...', 'info');
            
            fetch('/connect', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ ssid: ssid, password: password })
            })
            .then(response => response.json())
            .then(data => {
                if (data.success) {
                    showStatus('✅ Connected successfully! Device will restart...', 'success');
                    setTimeout(() => window.location.reload(), 3000);
                } else {
                    showStatus('❌ Connection failed: ' + data.message, 'error');
                }
            })
            .catch(error => {
                showStatus('❌ Request failed', 'error');
                console.error('Error:', error);
            });
        });

        function showStatus(message, type) {
            const status = document.getElementById('status');
            status.innerHTML = message;
            status.className = 'status ' + type;
        }

        // Initial network scan
        scanNetworks();
    </script>
</body>
</html>
)rawliteral";
}

