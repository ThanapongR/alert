#pragma once

#include <Arduino.h>

// ==========================================
// Hardware Pin Mapping (Seeed Studio XIAO ESP32-S3 + INMP441)
// ==========================================
// SCK  (BCLK)        -> D8  (GPIO 7)
// WS   (LRCK)        -> D9  (GPIO 8)
// SD   (Data In)     -> D10 (GPIO 9)
// L/R                -> GND (Left Channel)
// VDD                -> 3.3V
// GND                -> GND
#define I2S_SCK_PIN        7
#define I2S_WS_PIN         8
#define I2S_SD_PIN         9

// Onboard User LED on XIAO ESP32-S3 (Active LOW)
#define STATUS_LED_PIN     21
#define LED_ACTIVE_LEVEL   LOW

// ==========================================
// Audio DSP Parameters (MFE 16kHz)
// ==========================================
#define AUDIO_SAMPLE_RATE  16000
#define AUDIO_BUFFER_SIZE  1024

// ==========================================
// Dual-stage State Machine & Gating Parameters
// ==========================================
#define CONFIDENCE_THRESHOLD         0.90f
#define MIN_ALARM_RMS_THRESHOLD      400.0f // Optimal sensitivity for test speakers and real alarms
#define SUSPECTED_WINDOW_MS          16000  // 16 seconds window to allow hits across 2-3 T3 cycles
#define REQUIRED_HITS_FOR_ALERT      12     // 12 additional hits while suspected (total 13 hits)
#define REQUIRED_T3_CYCLES           2      // Minimum verified T3 cycles (triplet + 1.5s silence) for Alert
#define T3_SHORT_GAP_MIN_MS          250    // Gap between beeps inside triplet (~500ms)
#define T3_SHORT_GAP_MAX_MS          950
#define T3_LONG_GAP_MIN_MS           1000   // Inter-burst pause between cycles (~1500ms)
#define T3_LONG_GAP_MAX_MS           2600
#define CLEAR_RESET_TIMEOUT_MS       10000  // 10 seconds of silence to reset to Clear

// ==========================================
// Home Assistant MQTT Configuration
// ==========================================
#define DEVICE_ID             "xiao_alarm_detector"
#define DEVICE_NAME           "Alarm Detector"
#define DEVICE_MANUFACTURER   "Seeed Studio / Edge Impulse"
#define DEVICE_MODEL          "XIAO-ESP32S3-INMP441"
#define SW_VERSION            "1.0.0"

#define HA_DISCOVERY_PREFIX   "homeassistant"
#define MQTT_BASE_TOPIC       "homeassistant/sensor/" DEVICE_ID

#define TOPIC_STATE           MQTT_BASE_TOPIC "/state"
#define TOPIC_ATTRIBUTES      MQTT_BASE_TOPIC "/attributes"
#define TOPIC_AVAILABILITY    MQTT_BASE_TOPIC "/availability"
