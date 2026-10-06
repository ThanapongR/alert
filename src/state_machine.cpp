#include "state_machine.h"

DetectorStateMachine::DetectorStateMachine()
    : current_state(STATE_CLEAR),
      change_callback(nullptr),
      last_confidence(0.0f),
      consecutive_hits(0),
      triplet_beeps(0),
      t3_cycles(0),
      suspected_start_time(0),
      last_hit_time(0) {}

void DetectorStateMachine::begin(StateChangeCallback on_change) {
    this->change_callback = on_change;
    this->current_state = STATE_CLEAR;
    this->consecutive_hits = 0;
    this->triplet_beeps = 0;
    this->t3_cycles = 0;
    this->last_confidence = 0.0f;
    this->last_hit_time = 0;
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
        unsigned long gap = (last_hit_time > 0) ? (now - last_hit_time) : 0;
        last_hit_time = now;

        // T3 Cadence Analysis:
        // Gap < 250ms: Continuous beep slice (same burst)
        // Gap 250ms - 950ms: Next beep in triplet (~0.5s silence)
        // Gap 1000ms - 2600ms: Inter-burst pause (~1.5s silence)
        if (gap >= T3_SHORT_GAP_MIN_MS && gap <= T3_SHORT_GAP_MAX_MS) {
            triplet_beeps++;
        } else if (gap >= T3_LONG_GAP_MIN_MS && gap <= T3_LONG_GAP_MAX_MS) {
            if (triplet_beeps >= 2) {
                t3_cycles++;
                Serial.printf("\n🔔 >>> [T3 CADENCE VERIFIED] Cycle %u/%u detected! <<<\n\n", 
                              t3_cycles, REQUIRED_T3_CYCLES);
            }
            triplet_beeps = 1;
        } else if (gap > T3_LONG_GAP_MAX_MS) {
            triplet_beeps = 1;
        }

        switch (current_state) {
            case STATE_CLEAR:
                // First hit: transition to Suspected
                consecutive_hits = 1;
                triplet_beeps = 1;
                t3_cycles = 0;
                suspected_start_time = now;
                transitionTo(STATE_SUSPECTED, fire_alarm_confidence);
                break;

            case STATE_SUSPECTED:
                consecutive_hits++;
                // Verify within window: requires both total hits and 2 verified T3 cycles
                if ((now - suspected_start_time) <= SUSPECTED_WINDOW_MS) {
                    if (consecutive_hits >= (1 + REQUIRED_HITS_FOR_ALERT) && t3_cycles >= REQUIRED_T3_CYCLES) {
                        transitionTo(STATE_ALERT, fire_alarm_confidence);
                    }
                } else {
                    // Window expired without reaching full Alert, reset window
                    consecutive_hits = 1;
                    triplet_beeps = 1;
                    t3_cycles = 0;
                    suspected_start_time = now;
                }
                break;

            case STATE_ALERT:
                consecutive_hits++;
                break;
        }
    }
}

void DetectorStateMachine::update() {
    unsigned long now = millis();

    switch (current_state) {
        case STATE_SUSPECTED:
            // If suspected window elapsed without completing T3 cycles -> reset to Clear
            if ((now - suspected_start_time) > SUSPECTED_WINDOW_MS) {
                consecutive_hits = 0;
                triplet_beeps = 0;
                t3_cycles = 0;
                transitionTo(STATE_CLEAR, last_confidence);
            }
            break;

        case STATE_ALERT:
            // If silence timeout elapsed without any new hit -> reset to Clear
            if ((now - last_hit_time) > CLEAR_RESET_TIMEOUT_MS) {
                consecutive_hits = 0;
                triplet_beeps = 0;
                t3_cycles = 0;
                transitionTo(STATE_CLEAR, 0.0f);
            }
            break;

        case STATE_CLEAR:
            break;
    }
}

float DetectorStateMachine::getAlertProgress() const {
    if (current_state == STATE_CLEAR) return 0.0f;
    if (current_state == STATE_ALERT) return 1.0f;

    // Linear progress inside STATE_SUSPECTED based on hits and cycles verified
    float hit_prog = (float)consecutive_hits / (float)(1 + REQUIRED_HITS_FOR_ALERT);
    float cycle_prog = (float)t3_cycles / (float)REQUIRED_T3_CYCLES;
    
    // Balanced combination of hit count and T3 cycle cadence
    float prog = 0.5f * hit_prog + 0.5f * cycle_prog;
    if (prog < 0.0f) return 0.0f;
    if (prog > 1.0f) return 1.0f;
    return prog;
}
