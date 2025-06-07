#ifndef OMNISOURCEROUTER_H
#define OMNISOURCEROUTER_H

#include <string>
#include <udpManager.h>
#include <ESPAsyncWebServer.h>  // ESPAsyncWebServer library
#include <AsyncWebSocket.h>     // AsyncWebSocket from ESPAsyncWebServer
#include <WebSocketsClient.h>   // For WebSocket client (outgoing connections)
#include <ArduinoJson.h>
#include <functional>
#include <vector>
#include <Arduino.h>
#include <networkManager.h>

struct OmniSourceRouterCallback {
    String target;
    std::function<void(JsonObject&)> callback;
    unsigned long cooldownMs;

    OmniSourceRouterCallback(const String& tgt, std::function<void(JsonObject&)> cb, unsigned long cooldown = 1000)
        : target(tgt), callback(cb), cooldownMs(cooldown) {}
};

struct CooldownEntry {
    String target;
    unsigned long lastCallTime;
    unsigned long cooldownMs;
    JsonDocument pendingData;
    bool hasPendingCall;
    
    CooldownEntry() : lastCallTime(0), cooldownMs(1000), hasPendingCall(false) {}
    CooldownEntry(const String& tgt, unsigned long cooldown = 1000) 
        : target(tgt), lastCallTime(0), cooldownMs(cooldown), hasPendingCall(false) {}
};

class CooldownManager {
private:
    std::vector<CooldownEntry> cooldownEntries;
    
    // Helper function to find entry by target
    CooldownEntry* findEntry(const String& target) {
        for (auto& entry : cooldownEntries) {
            if (entry.target == target) {
                return &entry;
            }
        }
        return nullptr;
    }
    
public:
    // Set cooldown for a specific target (in milliseconds)
    void setCooldown(const String& target, unsigned long cooldownMs) {
        CooldownEntry* entry = findEntry(target);
        if (entry) {
            entry->cooldownMs = cooldownMs;
        } else {
            cooldownEntries.push_back(CooldownEntry(target, cooldownMs));
        }
    }
    
    // Execute callback with cooldown logic
    bool executeWithCooldown(const OmniSourceRouterCallback& callback, JsonObject& data) {
        String target = callback.target;
        unsigned long currentTime = millis();
        
        // Find or create entry
        CooldownEntry* entry = findEntry(target);
        if (!entry) {
            cooldownEntries.push_back(CooldownEntry(target, callback.cooldownMs));
            entry = &cooldownEntries.back();
        }
        
        // Check if we're still in cooldown period
        if (currentTime - entry->lastCallTime < entry->cooldownMs && entry->lastCallTime != 0) {
            // Store the new call data (override any previous pending call)
            entry->pendingData.clear();
            entry->pendingData.to<JsonObject>().set(data);
            entry->hasPendingCall = true;
            return false; // Call was queued, not executed
        }
        
        // Execute immediately
        callback.callback(data);
        entry->lastCallTime = currentTime;
        entry->hasPendingCall = false;
        
        return true; // Call was executed
    }
    
    // Call this regularly in your main loop to handle pending calls
    void processPendingCalls(const std::vector<OmniSourceRouterCallback>& callbacks) {
        unsigned long currentTime = millis();
        
        for (auto& entry : cooldownEntries) {
            // Check if cooldown period has passed and there's a pending call
            if (entry.hasPendingCall && 
                (currentTime - entry.lastCallTime >= entry.cooldownMs)) {
                
                // Find the callback for this target
                for (const auto& callback : callbacks) {
                    if (callback.target == entry.target) {
                        JsonObject pendingData = entry.pendingData.as<JsonObject>();
                        callback.callback(pendingData);
                        entry.lastCallTime = currentTime;
                        entry.hasPendingCall = false;
                        break;
                    }
                }
            }
        }
    }
    
    // Get remaining cooldown time for a target (returns 0 if not in cooldown)
    unsigned long getRemainingCooldown(const String& target) {
        CooldownEntry* entry = findEntry(target);
        if (!entry) return 0;
        
        unsigned long elapsed = millis() - entry->lastCallTime;
        if (elapsed >= entry->cooldownMs) return 0;
        
        return entry->cooldownMs - elapsed;
    }
    
    // Check if target has a pending call
    bool hasPendingCall(const String& target) {
        CooldownEntry* entry = findEntry(target);
        return entry && entry->hasPendingCall;
    }
};

class OmniSourceRouter {
public:
    OmniSourceRouter(NetworkManager* nm);
    ~OmniSourceRouter();
    NetworkManager* nm;
    
    // Components
    
    char incomingUDPPacket[255];
    
    // Public methods
    void inspectBody(JsonObject body);
    void addSource(const std::string& sourceName);
    void removeSource(const std::string& sourceName);
    void routeData(const std::string& data);
    void handle();
    
    // Enhanced callback methods with cooldown support
    void addCallback(OmniSourceRouterCallback cb);
    void addCallback(const String& target, std::function<void(JsonObject&)> callback, unsigned long cooldownMs = 1000);
    void delCallback(String target);
    
    // Cooldown utility methods
    void update(); // Call this in your main loop to process pending calls
    unsigned long getRemainingCooldown(const String& target);
    bool hasPendingCall(const String& target);
    
    bool httpStarted = false;
    
    // Static instance pointer for callbacks
    static OmniSourceRouter* instance;
    void begin();
    
private:
    std::vector<std::string> sources;
    std::vector<OmniSourceRouterCallback> routerCallbacks;
    CooldownManager cooldownManager; // Integrated cooldown system
    
    // Helper function to find callback by target
    OmniSourceRouterCallback* findCallback(const String& target) {
        for (auto& callback : routerCallbacks) {
            if (callback.target == target) {
                return &callback;
            }
        }
        return nullptr;
    }
    
    // Private methods
    void setupHttpRoutes();
    void setupWebSocketServer();
    void webSocketEvent(WStype_t type, uint8_t * payload, size_t length);
    static void webSocketEventStatic(WStype_t type, uint8_t * payload, size_t length);
    
    // WebSocket server event handler
    void onWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);
    
    // WebSocket client connection parameters (for outgoing connections)
    const char* ws_host = "your_websocket_host";
    const int ws_port = 80;
    const char* ws_path = "/";
};

#endif // OMNISOURCEROUTER_H