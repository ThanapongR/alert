#include <Arduino.h>
#include "config.h"
#include "state_machine.h"
#include "led_indicator.h"
#include "mqtt_ha.h"
#include "audio_infer.h"

static DetectorStateMachine state_machine;
static LedIndicator led;
static MqttHaManager mqtt;
static AudioInfer audio;

void onStateChange(SystemState old_state, SystemState new_state, float confidence) {
    const char* old_s = (old_state == STATE_CLEAR ? "CLEAR" : (old_state == STATE_SUSPECTED ? "SUSPECTED" : "ALERT"));
    const char* new_s = (new_state == STATE_CLEAR ? "CLEAR" : (new_state == STATE_SUSPECTED ? "SUSPECTED" : "ALERT"));
    Serial.printf("\n🚨 >>> [STATE TRANSITION] %s -> %s (Confidence: %.2f, Hits: %u) <<<\n\n", 
                  old_s, new_s, confidence, state_machine.getConsecutiveHits());

    // Update LED mode
    switch (new_state) {
        case STATE_CLEAR:     led.setMode(LED_MODE_CLEAR); break;
        case STATE_SUSPECTED: led.setMode(LED_MODE_SUSPECTED); break;
        case STATE_ALERT:     led.setMode(LED_MODE_ALERT); break;
    }

    // Publish state to Home Assistant
    mqtt.publishState(new_state, confidence, state_machine.getConsecutiveHits());
}


void setup() {
    Serial.begin(115200);
    delay(1500);

    Serial.println("==================================================");
    Serial.println(" XIAO ESP32-S3 Acoustic Fire Alarm Detector");
    Serial.println(" Protocol: MQTT Home Assistant Auto-Discovery");
    Serial.println(" Model: Edge Impulse 16kHz MFE TinyML");
    Serial.println("==================================================");

    led.begin();
    led.setMode(LED_MODE_CONNECTING);

    state_machine.begin(onStateChange);
    audio.begin();
    mqtt.begin();
}

void loop() {
    // 1. Maintain LED and Network loops
    led.update();
    mqtt.loop();

    // Maintain LED status based on priority: ALERT > SUSPECTED > CONNECTING > CLEAR
    SystemState current_state = state_machine.getCurrentState();
    if (current_state == STATE_ALERT) {
        led.setMode(LED_MODE_ALERT);
    } else if (current_state == STATE_SUSPECTED) {
        led.setMode(LED_MODE_SUSPECTED);
    } else if (!mqtt.isConnected()) {
        led.setMode(LED_MODE_CONNECTING);
    } else {
        led.setMode(LED_MODE_CLEAR);
    }

    // 2. Run audio sampling and inference
    float confidence = audio.runInference();
    if (confidence >= 0.0f) {
        state_machine.processInference(confidence);
    }

    // 3. Periodic state machine timeout check (Suspected window & Clear reset)
    state_machine.update();
}
