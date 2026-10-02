#include "audio_infer.h"
#include <driver/i2s.h>
#include <math.h>

#define I2S_PORT            I2S_NUM_0
#define DMA_BUFFER_SAMPLES  512

// Check if Edge Impulse project library is present in lib/
#if __has_include(<edge-impulse-sdk/classifier/ei_run_classifier.h>)
#include <edge-impulse-sdk/classifier/ei_run_classifier.h>
#define HAS_EDGE_IMPULSE 1
#else
#define HAS_EDGE_IMPULSE 0
#endif

static int32_t raw_i2s_buffer[DMA_BUFFER_SAMPLES];

#if HAS_EDGE_IMPULSE
static signed short *sample_buffer = nullptr;
static size_t sample_buffer_idx = 0;

static int audio_signal_get_data(size_t offset, size_t length, float *out_ptr) {
    numpy::int16_to_float(&sample_buffer[offset], out_ptr, length);
    return 0;
}
#endif

AudioInfer::AudioInfer()
    : current_rms(0.0f),
      i2s_initialized(false) {}

void AudioInfer::setupI2s() {
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
        Serial.printf("[Audio] Failed to install I2S driver: %d\n", err);
        return;
    }

    err = i2s_set_pin(I2S_PORT, &pin_config);
    if (err != ESP_OK) {
        Serial.printf("[Audio] Failed to set I2S pins: %d\n", err);
        return;
    }

    i2s_zero_dma_buffer(I2S_PORT);
    i2s_initialized = true;
    Serial.println("[Audio] I2S 16kHz initialized successfully.");
}

bool AudioInfer::begin() {
    setupI2s();

#if HAS_EDGE_IMPULSE
    Serial.printf("[ML] Edge Impulse SDK detected. Model: %s\n", EI_CLASSIFIER_PROJECT_NAME);
    run_classifier_init();
    sample_buffer = (signed short *)malloc(EI_CLASSIFIER_RAW_SAMPLE_COUNT * sizeof(signed short));
    if (!sample_buffer) {
        Serial.println("[ML] Error: Failed to allocate sample buffer!");
        return false;
    }
#else
    Serial.println("[ML] Notice: Edge Impulse library not yet placed in lib/ folder.");
    Serial.println("[ML] Running in Audio RMS monitor mode. Extract your Edge Impulse C++ library into lib/ to enable ML.");
#endif

    return i2s_initialized;
}

float AudioInfer::runInference() {
    if (!i2s_initialized) return -1.0f;

    size_t bytes_read = 0;
    esp_err_t result = i2s_read(I2S_PORT, raw_i2s_buffer, sizeof(raw_i2s_buffer), &bytes_read, 10 / portTICK_PERIOD_MS);

    if (result != ESP_OK || bytes_read == 0) {
        return -1.0f;
    }

    size_t samples_read = bytes_read / sizeof(int32_t);
    int64_t sum_squares = 0;

    for (size_t i = 0; i < samples_read; i++) {
        int16_t sample16 = (int16_t)(raw_i2s_buffer[i] >> 14);
        sum_squares += (int32_t)sample16 * (int32_t)sample16;
    }

    // Compute RMS
    current_rms = sqrtf((float)sum_squares / samples_read);

#if HAS_EDGE_IMPULSE
    // Fill audio buffer and run Edge Impulse classifier
    signal_t signal;
    signal.total_length = EI_CLASSIFIER_RAW_SAMPLE_COUNT;
    signal.get_data = &audio_signal_get_data;

    // In full continuous mode, run_classifier returns results
    ei_impulse_result_t ei_result = { 0 };
    EI_IMPULSE_ERROR r = run_classifier(&signal, &ei_result, false);
    if (r != EI_IMPULSE_OK) {
        return -1.0f;
    }

    // Search for fire_alarm_t3 class confidence
    for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
        if (strcmp(ei_result.classification[ix].label, "fire_alarm_t3") == 0) {
            return ei_result.classification[ix].value;
        }
    }
#endif

    return -1.0f;
}
