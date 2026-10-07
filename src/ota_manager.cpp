#include "ota_manager.h"
#include "config.h"
#include <ArduinoOTA.h>

OtaManager ota_manager;

OtaManager::OtaManager()
    : initialized(false),
      updating(false) {}

void OtaManager::begin() {
    if (initialized) return;

    ArduinoOTA.setHostname(DEVICE_ID);

    ArduinoOTA.onStart([this]() {
        updating = true;
        String type;
        if (ArduinoOTA.getCommand() == U_FLASH) {
            type = "sketch";
        } else {
            type = "filesystem";
        }
        Serial.printf("\n[OTA] Start updating %s...\n", type.c_str());
    });

    ArduinoOTA.onEnd([this]() {
        updating = false;
        Serial.println("\n[OTA] Update complete! Rebooting...");
    });

    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        static unsigned int last_percent = 0;
        unsigned int percent = (progress / (total / 100));
        if (percent != last_percent && percent % 10 == 0) {
            last_percent = percent;
            Serial.printf("[OTA] Progress: %u%%\n", percent);
        }
    });

    ArduinoOTA.onError([this](ota_error_t error) {
        updating = false;
        Serial.printf("[OTA] Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
        else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
        else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
        else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
        else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });

    ArduinoOTA.begin();
    initialized = true;
    Serial.printf("[OTA] ArduinoOTA service initialized (Hostname: %s.local)\n", DEVICE_ID);
}

void OtaManager::update() {
    if (initialized) {
        ArduinoOTA.handle();
    }
}
