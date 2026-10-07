#include "mqtt_ha.h"

MqttHaManager::MqttHaManager()
    : mqttClient(espClient),
      last_reconnect_attempt(0),
      last_heartbeat(0) {}

void MqttHaManager::begin() {
    mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
    // Increase buffer size for discovery JSON payloads
    mqttClient.setBufferSize(1024);

    connectWifi();
}

void MqttHaManager::connectWifi() {
    if (WiFi.status() == WL_CONNECTED) return;

    Serial.printf("[WiFi] Connecting to %s...\n", WIFI_SSID);
    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false); // Disable modem sleep to prevent RF power fluctuations into I2S mic
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

bool MqttHaManager::connectMqtt() {
    if (mqttClient.connected()) return true;

    Serial.print("[MQTT] Connecting to broker...");

    // Last Will and Testament configuration
    const char* will_topic = TOPIC_AVAILABILITY;
    uint8_t will_qos = 1;
    bool will_retain = true;
    const char* will_message = "offline";

    bool success = false;
    if (strlen(MQTT_USER) > 0) {
        success = mqttClient.connect(DEVICE_ID, MQTT_USER, MQTT_PASSWORD, will_topic, will_qos, will_retain, will_message);
    } else {
        success = mqttClient.connect(DEVICE_ID, will_topic, will_qos, will_retain, will_message);
    }

    if (success) {
        Serial.println(" Connected!");
        // Publish online availability
        mqttClient.publish(TOPIC_AVAILABILITY, "online", true);
        // Publish HA Auto-Discovery definitions
        publishDiscovery();
        // Send initial clear state
        publishState(STATE_CLEAR, 0.0f, 0);
    } else {
        Serial.printf(" Failed, rc=%d\n", mqttClient.state());
    }

    return success;
}

void MqttHaManager::publishDiscovery() {
    Serial.println("[HA] Publishing MQTT Auto-Discovery topics...");

    // 1. Binary Sensor (Alarm)
    {
        JsonDocument doc;
        doc["name"] = "Alarm";
        doc["has_entity_name"] = true;
        doc["unique_id"] = String(DEVICE_ID) + "_alarm";
        doc["state_topic"] = TOPIC_STATE;
        doc["value_template"] = "{{ 'ON' if value_json.state == 'alert' else 'OFF' }}";
        doc["device_class"] = "safety";
        doc["availability_topic"] = TOPIC_AVAILABILITY;
        doc["payload_available"] = "online";
        doc["payload_not_available"] = "offline";

        JsonObject dev = doc["device"].to<JsonObject>();
        JsonArray ids = dev["identifiers"].to<JsonArray>();
        ids.add(DEVICE_ID);
        dev["name"] = DEVICE_NAME;
        dev["model"] = DEVICE_MODEL;
        dev["manufacturer"] = DEVICE_MANUFACTURER;
        dev["sw_version"] = SW_VERSION;

        char buffer[768];
        serializeJson(doc, buffer);
        String discovery_topic = String(HA_DISCOVERY_PREFIX) + "/binary_sensor/" + DEVICE_ID + "_alarm/config";
        mqttClient.publish(discovery_topic.c_str(), buffer, true);
    }

    // 2. Sensor (State & Diagnostics)
    {
        JsonDocument doc;
        doc["name"] = "State";
        doc["has_entity_name"] = true;
        doc["unique_id"] = String(DEVICE_ID) + "_state";
        doc["state_topic"] = TOPIC_STATE;
        doc["value_template"] = "{{ value_json.state }}";
        doc["json_attributes_topic"] = TOPIC_ATTRIBUTES;
        doc["icon"] = "mdi:bell-alert";
        doc["availability_topic"] = TOPIC_AVAILABILITY;
        doc["payload_available"] = "online";
        doc["payload_not_available"] = "offline";

        JsonObject dev = doc["device"].to<JsonObject>();
        JsonArray ids = dev["identifiers"].to<JsonArray>();
        ids.add(DEVICE_ID);
        dev["name"] = DEVICE_NAME;
        dev["model"] = DEVICE_MODEL;
        dev["manufacturer"] = DEVICE_MANUFACTURER;
        dev["sw_version"] = SW_VERSION;

        char buffer[768];
        serializeJson(doc, buffer);
        String discovery_topic = String(HA_DISCOVERY_PREFIX) + "/sensor/" + DEVICE_ID + "_state/config";
        mqttClient.publish(discovery_topic.c_str(), buffer, true);
    }
}

void MqttHaManager::publishState(SystemState state, float confidence, uint32_t hits) {
    if (!mqttClient.connected()) return;

    const char* state_str = "clear";
    if (state == STATE_SUSPECTED) state_str = "suspected";
    else if (state == STATE_ALERT) state_str = "alert";

    // Publish state payload
    JsonDocument state_doc;
    state_doc["state"] = state_str;
    char state_buf[128];
    serializeJson(state_doc, state_buf);
    mqttClient.publish(TOPIC_STATE, state_buf, true);

    // Publish attributes
    JsonDocument attr_doc;
    attr_doc["state"] = state_str;
    attr_doc["confidence"] = serialized(String(confidence, 2));
    attr_doc["consecutive_hits"] = hits;
    attr_doc["wifi_rssi"] = WiFi.RSSI();
    attr_doc["free_heap"] = ESP.getFreeHeap();
    attr_doc["uptime_sec"] = millis() / 1000;

    char attr_buf[256];
    serializeJson(attr_doc, attr_buf);
    mqttClient.publish(TOPIC_ATTRIBUTES, attr_buf, false);

    Serial.printf("[MQTT] State published: %s (Confidence: %.2f, Hits: %u)\n", state_str, confidence, hits);
}

void MqttHaManager::publishHeartbeat() {
    if (!mqttClient.connected()) return;

    JsonDocument attr_doc;
    attr_doc["wifi_rssi"] = WiFi.RSSI();
    attr_doc["free_heap"] = ESP.getFreeHeap();
    attr_doc["uptime_sec"] = millis() / 1000;

    char attr_buf[192];
    serializeJson(attr_doc, attr_buf);
    mqttClient.publish(TOPIC_ATTRIBUTES, attr_buf, false);
}

bool MqttHaManager::isConnected() {
    return WiFi.status() == WL_CONNECTED && mqttClient.connected();
}

void MqttHaManager::loop() {
    unsigned long now = millis();

    if (WiFi.status() != WL_CONNECTED) {
        if (now - last_reconnect_attempt > 5000) {
            last_reconnect_attempt = now;
            connectWifi();
        }
        return;
    }

    if (!mqttClient.connected()) {
        if (now - last_reconnect_attempt > 5000) {
            last_reconnect_attempt = now;
            connectMqtt();
        }
    } else {
        mqttClient.loop();

        // Publish heartbeat every 30 seconds
        if (now - last_heartbeat >= 30000) {
            last_heartbeat = now;
            publishHeartbeat();
        }
    }
}
