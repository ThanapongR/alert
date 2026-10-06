#include "led_indicator.h"

LedIndicator::LedIndicator()
    : current_mode(LED_MODE_CONNECTING), last_toggle(0), led_state(false), suspected_progress(0.0f) {}

void LedIndicator::begin() {
  pinMode(STATUS_LED_PIN, OUTPUT);
  setLed(false);
}

void LedIndicator::setLed(bool on) {
  led_state = on;
  digitalWrite(STATUS_LED_PIN, on ? LED_ACTIVE_LEVEL : !LED_ACTIVE_LEVEL);
}

void LedIndicator::setMode(LedMode mode) {
  if (current_mode != mode) {
    current_mode = mode;
    last_toggle = millis();
    if (mode == LED_MODE_ALERT) {
      setLed(true);
    } else if (mode == LED_MODE_CLEAR) {
      setLed(false);
      suspected_progress = 0.0f;
    }
  }
}

void LedIndicator::setSuspectedProgress(float progress) {
  suspected_progress = (progress < 0.0f) ? 0.0f : ((progress > 1.0f) ? 1.0f : progress);
}

void LedIndicator::update() {
  unsigned long now = millis();

  switch (current_mode) {
  case LED_MODE_CONNECTING:
    // Slow blink (1 Hz): 500ms on, 500ms off
    if (now - last_toggle >= 500) {
      last_toggle = now;
      setLed(!led_state);
    }
    break;

  case LED_MODE_CLEAR:
    // Completely OFF when Clear
    setLed(false);
    break;

  case LED_MODE_SUSPECTED: {
    // Dynamic linear acceleration: from 350ms (slow warning) down to 40ms (fast frenzy) as confidence builds up
    unsigned long interval = 350 - (unsigned long)(suspected_progress * 310.0f);
    if (now - last_toggle >= interval) {
      last_toggle = now;
      setLed(!led_state);
    }
    break;
  }

  case LED_MODE_ALERT:
    // Solid ON
    setLed(true);
    break;
  }
}
