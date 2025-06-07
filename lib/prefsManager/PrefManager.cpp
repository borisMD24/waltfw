#include "PrefManager.h"
#include <FS.h>           // File System générique
#include <SPIFFS.h>       // SPIFFS pour ESP32
#include <ArduinoJson.h>

PrefManager::PrefManager() {
    if (!SPIFFS.begin(true)) {
        Serial.println("SPIFFS Mount Failed");
        return;
    }
    Serial.println("SPIFFS Ready !");
}

PrefManager& PrefManager::getInstance() {
    static PrefManager instance;
    return instance;
}

void PrefManager::write(const char* to, JsonDocument body) {
    String path = String("/") + to;  // SPIFFS veut des chemins absolus sans "./"
    File file = SPIFFS.open(path, FILE_WRITE);
    
    if (!file) {
        Serial.println("Erreur d'ouverture de fichier !");
        return;
    }

    String stringyfied;
    serializeJson(body, stringyfied);  // Convertir le JSON en String
    file.print(stringyfied);           // Écrire tout le body d’un coup
    file.close();
    Serial.println("Fichier écrit !");
}


DynamicJsonDocument PrefManager::read(const char* from) {
    File file = SPIFFS.open(String("/") + from, FILE_READ);
    DynamicJsonDocument ret(1024);  // alloue 1 Ko dynamiquement
    
    if (!file) {
        Serial.println("Erreur de lecture !");
        return ret;
    }

    DeserializationError error = deserializeJson(ret, file);
    if (error) {
        Serial.print("Erreur de désérialisation : ");
        Serial.println(error.f_str());
    } else {
        Serial.println("✅ Fichier JSON chargé !");
    }

    return ret;
}
