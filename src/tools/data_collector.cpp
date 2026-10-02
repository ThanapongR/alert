/**
 * @file data_collector.cpp
 * @brief In-situ Audio Data Collector for Edge Impulse Data Forwarder CLI
 * 
 * Collects 16 kHz audio from INMP441 via I2S and streams 16-bit signed samples
 * to USB Serial so that `edge-impulse-data-forwarder` can record training datasets.
 */

#include <Arduino.h>
#include <driver/i2s.h>
#include "config.h"

#define I2S_PORT            I2S_NUM_0
#define DMA_BUFFER_SAMPLES  256

static int32_t raw_i2s_buffer[DMA_BUFFER_SAMPLES];

void setup_i2s() {
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
        .sample_rate = AUDIO_SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 4,
        .dma_buf_len = DMA_BUFFER_SAMPLES,
        .use_apll = false,
        .tx_desc_auto_clear = false,
        .fixed_mclk = 0
    };

    i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_SCK_PIN,
        .ws_io_num = I2S_WS_PIN,
        .data_out_num = I2S_PIN_NO_CHANGE,
        .data_in_num = I2S_SD_PIN
    };

    esp_err_t err = i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
    if (err != ESP_OK) {
        Serial.printf("Failed to install I2S driver: %d\n", err);
        while (1) { delay(1000); }
    }

    err = i2s_set_pin(I2S_PORT, &pin_config);
    if (err != ESP_OK) {
        Serial.printf("Failed to set I2S pins: %d\n", err);
        while (1) { delay(1000); }
    }

    i2s_zero_dma_buffer(I2S_PORT);
}

void setup() {
    Serial.begin(115200);
    // Allow USB CDC on ESP32-S3 to initialize
    delay(2000);

    pinMode(STATUS_LED_PIN, OUTPUT);
    digitalWrite(STATUS_LED_PIN, !LED_ACTIVE_LEVEL); // LED Off

    setup_i2s();
}

void loop() {
    size_t bytes_read = 0;
    esp_err_t result = i2s_read(I2S_PORT, raw_i2s_buffer, sizeof(raw_i2s_buffer), &bytes_read, portMAX_DELAY);

    if (result == ESP_OK && bytes_read > 0) {
        size_t samples_read = bytes_read / sizeof(int32_t);
        for (size_t i = 0; i < samples_read; i++) {
            // INMP441 24-bit data in 32-bit slot; shift right by 14 to convert to 16-bit PCM with headroom
            int16_t sample16 = (int16_t)(raw_i2s_buffer[i] >> 14);
            // Format for edge-impulse-data-forwarder: one sample per line
            Serial.println(sample16);
        }
    }
}
