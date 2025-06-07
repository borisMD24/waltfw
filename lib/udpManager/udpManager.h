#ifndef UDPMANAGER_H
#define UDPMANAGER_H

#include <WiFiUdp.h>
#include <Arduino.h>
#include <functional>

#define MAX_SOURCES 10

class UDPManager {
public:
    UDPManager();
    ~UDPManager();

    void addSource(const String& sourceName);
    void removeSource(const String& sourceName);
    void routeData(const String& data);

    void beginUDP(int port);
    void processUDP();
    void sendData(const String& ip, int port, const String& data);

    // Subscribe avec une lambda/callback
    void subscribe(std::function<void(const String&)> callback);

private:
    WiFiUDP udp;
    String sources[MAX_SOURCES];
    int sourceCount = 0;

    std::function<void(const String&)> subscriptionCallback;
    bool subscribed = false;
};

#endif // UDPMANAGER_H
