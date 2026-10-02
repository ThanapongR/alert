#include "state_machine.h"

DetectorStateMachine::DetectorStateMachine()
    : current_state(STATE_CLEAR),
      change_callback(nullptr),
      last_confidence(0.0f),
      consecutive_hits(0),
      suspected_start_time(0),
      last_hit_time(0) {}

void DetectorStateMachine::begin(StateChangeCallback on_change) {
    this->change_callback = on_change;
    this->current_state = STATE_CLEAR;
    this->consecutive_hits = 0;
    this->last_confidence = 0.0f;
}

const char* DetectorStateMachine::getStateString() const {
    switch (current_state) {
        case STATE_CLEAR:     return "clear";
        case STATE_SUSPECTED: return "suspected";
        case STATE_ALERT:     return "alert";
        default:              return "unknown";
    }
}

void DetectorStateMachine::transitionTo(SystemState new_state, float confidence) {
    if (current_state != new_state) {
        SystemState old = current_state;
        current_state = new_state;
        if (change_callback) {
            change_callback(old, new_state, confidence);
        }
    }
}

void DetectorStateMachine::processInference(float fire_alarm_confidence) {
    unsigned long now = millis();
    last_confidence = fire_alarm_confidence;

    if (fire_alarm_confidence >= CONFIDENCE_THRESHOLD) {
        last_hit_time = now;

        switch (current_state) {
            case STATE_CLEAR:
                // First hit: transition to Suspected and start debounce window
                consecutive_hits = 1;
                suspected_start_time = now;
                transitionTo(STATE_SUSPECTED, fire_alarm_confidence);
                break;

            case STATE_SUSPECTED:
                consecutive_hits++;
                // Check if hits occurred within the allowed window
                if ((now - suspected_start_time) <= SUSPECTED_WINDOW_MS) {
                    if (consecutive_hits >= (1 + REQUIRED_HITS_FOR_ALERT)) {
                        transitionTo(STATE_ALERT, fire_alarm_confidence);
                    }
                } else {
                    // Window expired, reset window from this new hit
                    consecutive_hits = 1;
                    suspected_start_time = now;
                }
                break;

            case STATE_ALERT:
                // Already in alert, refresh hit time
                consecutive_hits++;
                break;
        }
    }
}

void DetectorStateMachine::update() {
    unsigned long now = millis();

    switch (current_state) {
        case STATE_SUSPECTED:
            // If suspected window elapsed without reaching required hits for Alert -> reset to Clear
            if ((now - suspected_start_time) > SUSPECTED_WINDOW_MS) {
                consecutive_hits = 0;
                transitionTo(STATE_CLEAR, last_confidence);
            }
            break;

        case STATE_ALERT:
            // If silence timeout elapsed without any new hit -> reset to Clear
            if ((now - last_hit_time) > CLEAR_RESET_TIMEOUT_MS) {
                consecutive_hits = 0;
                transitionTo(STATE_CLEAR, 0.0f);
            }
            break;

        case STATE_CLEAR:
            break;
    }
}
