#ifndef PREFMANAGER_H
#define PREFMANAGER_H
#include <ArduinoJson.h>

class PrefManager {
public:
    static PrefManager& getInstance();
    void write(const char* to, JsonDocument body); 
    DynamicJsonDocument read(const char* from);
    PrefManager(); // Constructeur privé
};

#endif // PREFMANAGER_H