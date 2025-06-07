#ifndef WSETUP_H
#define WSETUP_H

#include <ArduinoJson.h>
#include <PrefManager.h>
#include <omniSourceRouter.h>
#include <IOWrapper.h>
#include <networkManager.h>


class WSetup
{
private:
    NetworkManager* nm;
    DynamicJsonDocument config;
    PrefManager prefs;
    void parseIO(JsonObject io);
    void setupIO(DynamicJsonDocument io);
    void setupStrip(DynamicJsonDocument strip);
    void setupBtn(DynamicJsonDocument strip);
    IOWrapper* wrapper;
    uint8_t ledTypeFromString(const String& typeStr);
public:
    WSetup(IOWrapper* wrapper, NetworkManager* nm);
    ~WSetup();
};


#endif