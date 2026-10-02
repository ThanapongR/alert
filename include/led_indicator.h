#pragma once

#include <Arduino.h>
#include "config.h"
#include "state_machine.h"

enum LedMode {
    LED_MODE_CONNECTING,
    LED_MODE_CLEAR,
    LED_MODE_SUSPECTED,
    LED_MODE_ALERT
};

class LedIndicator {
public:
    LedIndicator();
    void begin();
    void setMode(LedMode mode);
    void update();

private:
    LedMode current_mode;
    unsigned long last_toggle;
    bool led_state;

    void setLed(bool on);
};
