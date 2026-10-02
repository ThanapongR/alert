#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "config.h"
#include "secrets.h"
#include "state_machine.h"

class MqttHaManager {
public:
    MqttHaManager();
    void begin();
    void loop();

    bool isConnected();
    void publishState(SystemState state, float confidence, uint32_t hits);
    void publishHeartbeat();

private:
    WiFiClient espClient;
    PubSubClient mqttClient;

    unsigned long last_reconnect_attempt;
    unsigned long last_heartbeat;

    void connectWifi();
    bool connectMqtt();
    void publishDiscovery();
};
