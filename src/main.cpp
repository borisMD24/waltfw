#include <Arduino.h>
#include <WiFi.h>
#include <Adafruit_NeoPixel.h>
#include <ArduinoJson.h>

// Internal Modules
#include <omniSourceRouter.h>
#include <PrefManager.h>
#include <wcolor.h>
#include <LEDStrip.h>
#include <NetworkManager.h>
#include <udpManager.h>
#include <header.hpp>
#include <WiFiCaptiveManager.h>
#include <EEPROM.h>
#include <wsetup.h>
#include <IOWrapper.h>
#include <ArduinoOTA.h>


const uint32_t HEARTBEAT_INTERVAL = 30000;
const uint32_t WIFI_CHECK_INTERVAL = 5000;
const uint16_t LED_COUNT = 15;
const uint8_t LED_PIN = 2;
const uint8_t LED_TYPE = NEO_GRB + NEO_KHZ800;
AsyncWebServer server(80);
DNSServer dns;
WiFiCaptiveManager captiveManager(server, dns);
// Global Instances
PrefManager pm;
// LEDStrip strip(LED_COUNT, LED_PIN, LED_TYPE);
NetworkManager nm;
OmniSourceRouter controller(&nm);
IOWrapper wrapper(&controller);
void blankEEPROM()
{
  Serial.println("Blanking entire EEPROM...");

  // Initialize EEPROM if not already done
  if (!EEPROM.begin(512))
  {
    Serial.println("EEPROM initialization failed");
    return;
  }

  // Write zeros to entire EEPROM
  for (int i = 0; i < 512; i++)
  {
    EEPROM.write(i, 0);

    // Show progress every 50 bytes
    if (i % 50 == 0)
    {
      Serial.printf("Blanking progress: %d/512 bytes\n", i);
    }
  }

  // Commit changes
  if (EEPROM.commit())
  {
    Serial.println("✅ EEPROM blanked successfully");
  }
  else
  {
    Serial.println("❌ Failed to commit EEPROM blanking");
  }

  Serial.println("EEPROM blanking complete");
}

void setup()
{
  Serial.begin(115200);
  // blankEEPROM();
  captiveManager.begin();
  if (!captiveManager.isCaptivePortalActive())
  {
    controller.begin();
  }
  WSetup setup(&wrapper, &nm);
  ArduinoOTA
      .onStart([]()
               {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH) type = "sketch";
      else type = "filesystem";
      Serial.println("Démarrage OTA - " + type);
      Serial.println("should call all apoptosis methods"); })
      .onEnd([]()
             { Serial.println("\nFin OTA"); })
      .onProgress([](unsigned int progress, unsigned int total)
                  { Serial.printf("Progression : %u%%\r", (progress / (total / 100))); })
      .onError([](ota_error_t error)
               {
      Serial.printf("Erreur OTA [%u] : ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed"); });
  ArduinoOTA.setPassword("BobinouTKT42");
  ArduinoOTA.begin();
  Serial.println("OTA prêt !");
}

void loop()
{
  ArduinoOTA.handle();
  if (captiveManager.isCaptivePortalActive())
  {
    captiveManager.loop();
  }
  else
  {
    controller.handle();
  }
  delay(10);
}