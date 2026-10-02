#pragma once

#include <Arduino.h>
#include "config.h"

enum SystemState {
    STATE_CLEAR = 0,
    STATE_SUSPECTED = 1,
    STATE_ALERT = 2
};

typedef void (*StateChangeCallback)(SystemState old_state, SystemState new_state, float confidence);

class DetectorStateMachine {
public:
    DetectorStateMachine();

    void begin(StateChangeCallback on_change);
    
    // Call this each time an inference completes with confidence of T3 alarm
    void processInference(float fire_alarm_confidence);
    
    // Call periodically in loop() to handle timeouts
    void update();

    SystemState getCurrentState() const { return current_state; }
    const char* getStateString() const;
    float getLastConfidence() const { return last_confidence; }
    uint32_t getConsecutiveHits() const { return consecutive_hits; }

private:
    SystemState current_state;
    StateChangeCallback change_callback;

    float last_confidence;
    uint32_t consecutive_hits;
    unsigned long suspected_start_time;
    unsigned long last_hit_time;

    void transitionTo(SystemState new_state, float confidence);
};
