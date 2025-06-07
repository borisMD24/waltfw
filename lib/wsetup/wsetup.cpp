#include "wsetup.h"
#include <ArduinoJson.h>
#include <PrefManager.h>
#include <IOWrapper.h>
#include <Adafruit_NeoPixel.h>
#include <pushBtn.h>
#include <networkManager.h>
WSetup::WSetup(IOWrapper* wrapper, NetworkManager* nm):prefs(),config(2048)
{
    this->nm = nm;
    this->wrapper = wrapper;
    this->config = prefs.read("config.json");
    Serial.println("📦 Config lue from wsetup:");
    serializeJsonPretty(this->config, Serial);  // ou serializeJson(doc, Serial);
    if (this->config.containsKey("ioIndex")) {
        if (this->config["ioIndex"].is<JsonArray>()) {
            JsonArray array = this->config["ioIndex"].as<JsonArray>();

            for (JsonObject io : array) {
                this->parseIO(io);
            }
        } else {
            Serial.println("🛑 ioIndex n'est pas un tableau !");
        }
    }
}


void WSetup::parseIO(JsonObject io){
    Serial.println("📦 Objet IO reçu :");
    serializeJsonPretty(io, Serial);
    Serial.println(); // Pour sauter une ligne
    String uid = io["UID"].as<String>();
    this->setupIO(this->prefs.read((uid+".json").c_str()));
}

void WSetup::setupIO(DynamicJsonDocument io){
    Serial.println("io json in setupio :");
    serializeJson(io, Serial);
    Serial.println();
    String type = io["type"].as<String>();
    if(type == "ledstrip"){
        this->setupStrip(io);
    }
    if(type == "btn"){
        this->setupBtn(io);
    }
}

void WSetup::setupStrip(DynamicJsonDocument strip){
    Serial.println("in setup strip ");
    Serial.println("********");
    Serial.println("********");
    Serial.println("********");
    if(strip.containsKey("UID") && 
       strip.containsKey("ledCount") && 
       strip.containsKey("pin") && 
       strip.containsKey("ledType")){
        String ledTypeStr = strip["ledType"].as<String>();
        uint8_t ledtype = this->ledTypeFromString(ledTypeStr);
        String uid = strip["UID"].as<String>();
        uint16_t ledcount = strip["ledCount"].as<uint16_t>();
        uint8_t pin = strip["pin"].as<uint8_t>();
        Serial.println("type : " + ledTypeStr);
        Serial.println(ledcount);
        Serial.println(pin);
        Serial.println("uid : " + uid);

        if(ledtype > 0 &&
            ledcount > 0 &&
            pin >= 0
            ){
                Serial.println("strip pushed");
                this->wrapper->pushOutput(new LEDStrip(ledcount, pin, ledtype), uid);
            }
       }
}

void WSetup::setupBtn(DynamicJsonDocument btn){
    Serial.println("in setup btn ");
    
    int pin = btn["pin"].as<int>();
    String uid = btn["UID"].as<String>();
    
    // Capture nm directly instead of this
    NetworkManager* networkManager = this->nm;
    
    wrapper->pushDigitalInput(new PushBtn(pin), uid, [uid, networkManager](DInput* btn){
        Serial.println("btn changed from setup");
        Serial.println(uid);
        String s = btn->getState() ? "true" : "false";
        if (networkManager) {
            networkManager->webSocket.sendTXT("btn changed from setup, UID : " + uid + ", state : " + s);
        } else {
            Serial.println("NetworkManager not available");
        }
    });
}

uint8_t WSetup::ledTypeFromString(const String& typeStr) {
    if(typeStr == "NEO_GRB + NEO_KHZ800") return NEO_GRB | NEO_KHZ800;
    if(typeStr == "NEO_GRB") return NEO_GRB;
    if(typeStr == "NEO_KHZ800") return NEO_KHZ800;
    return 0; // inconnu
}

WSetup::~WSetup()
{
}