#pragma once

#include <Arduino.h>
#include "config.h"

class AudioInfer {
public:
    AudioInfer();
    bool begin();
    
    // Reads I2S audio stream and runs inference when a slice is ready
    // Returns confidence of fire_alarm_t3 class (0.0 to 1.0)
    // Returns -1.0 if no new inference completed in this cycle
    float runInference();

    // Returns current audio RMS level for audio calibration
    float getRmsLevel() const { return current_rms; }

private:
    float current_rms;
    bool i2s_initialized;

    void setupI2s();
};
