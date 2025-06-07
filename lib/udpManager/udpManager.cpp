#include "udpManager.h"
#include <header.hpp>

UDPManager::UDPManager() {}
UDPManager::~UDPManager() {}

void UDPManager::addSource(const String& sourceName) {
    if (sourceCount < MAX_SOURCES) {
        sources[sourceCount++] = sourceName;
    }
}

void UDPManager::removeSource(const String& sourceName) {
    for (int i = 0; i < sourceCount; ++i) {
        if (sources[i] == sourceName) {
            for (int j = i; j < sourceCount - 1; ++j) {
                sources[j] = sources[j + 1];
            }
            sourceCount--;
            break;
        }
    }
}

void UDPManager::routeData(const String& data) {
    for (int i = 0; i < sourceCount; ++i) {
        Serial.print(F("Routing data to: "));
        Serial.println(sources[i]);
    }
}

void UDPManager::beginUDP(int port) {
    udp.begin(port);
    Serial.print(F("Serveur UDP démarré sur le port "));
    Serial.println(port);
}

void UDPManager::subscribe(std::function<void(const String&)> callback) {
    this->subscriptionCallback = callback;
    this->subscribed = true;
}

void UDPManager::processUDP() {
    int packetSize = udp.parsePacket();
    
    if (packetSize) {
        

        char buffer[256];
        int len = udp.read(buffer, 255);
        if (len <= 0) return;
        buffer[len] = '\0';
        Serial.println(buffer);
        if (len < 12) return;
        Header header;
        header.fromBytes((uint8_t*)buffer);
        this->sendData("192.168.1.18", 9999, buffer);

        if (this->subscribed && header.type == 5) {
            String payload = String(buffer + 12);
            this->subscriptionCallback(payload);
            Serial.println(buffer);
            this->sendData("192.168.1.18", 9999, payload);
        }
        if (header.type == 1){
            this->sendData("192.168.1.18", 9999, "hb");
        }
    }
}

void UDPManager::sendData(const String& udpAddress, int port, const String& data) {
    udp.beginPacket(udpAddress.c_str(), port);
    udp.write((const uint8_t*)data.c_str(), data.length());
    udp.endPacket();
}
