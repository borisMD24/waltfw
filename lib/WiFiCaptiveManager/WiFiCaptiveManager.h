#ifndef WIFICAPTIVEMANAGER_H
#define WIFICAPTIVEMANAGER_H

#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <DNSServer.h>
#include <ArduinoJson.h>

#define DNS_PORT 53

class WiFiCaptiveManager {
public:
    // Constructor
    WiFiCaptiveManager(AsyncWebServer& srv, DNSServer& dns);
    
    // Public methods
    void begin();
    void loop();
    void clearCredentials();
    bool isConnected() const { return WiFi.status() == WL_CONNECTED; }
    bool isCaptivePortalActive() const { return captivePortalActive; }
    String getConnectedSSID() const { return WiFi.SSID(); }
    IPAddress getLocalIP() const { return WiFi.localIP(); }
    IPAddress getAPIP() const { return WiFi.softAPIP(); }

private:
    // Private member variables
    AsyncWebServer& server;
    DNSServer& dnsServer;
    bool captivePortalActive;
    String ssid;
    String password;
    void sendCaptivePortalResponse(AsyncWebServerRequest *request);
    // EEPROM Configuration constants
    static const int EEPROM_SIZE = 512;
    static const int SSID_ADDR = 0;
    static const int SSID_MAX_LEN = 32;
    static const int PASSWORD_ADDR = 64;
    static const int PASSWORD_MAX_LEN = 64;
    static const int MAGIC_ADDR = 128;
    static const uint16_t MAGIC_VALUE = 0xABCD;
    
    // Private methods for credential management
    void loadCredentials();
    void saveCredentials(const String& newSSID, const String& newPassword);
    
    // Private methods for WiFi connection
    void connectToWiFi();
    
    // Private methods for captive portal
    void startCaptivePortal();
    void startAccessPoint();
    void setupWebServer();
    
    // Web server request handlers
    void handleScanNetworks(AsyncWebServerRequest *request);
    void handleConnect(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total);
    void handleCaptivePortalDetect(AsyncWebServerRequest *request);
    
    // HTML content
    const char* getCaptivePortalHTML();
};

#endif // WIFICAPTIVEMANAGER_H