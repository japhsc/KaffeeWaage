#include "buttons.h"

#include "config.h"

Buttons::Buttons(uint32_t longPressMs) : long_press_ms_(longPressMs) {}

void Buttons::begin(uint8_t pinStart) {
    ps_ = pinStart;
    pinMode(ps_, INPUT_PULLUP);
}

void Buttons::update() {
    bool v = digitalRead(ps_);
    if (v != last_) {
        t_ = millis();
        last_ = v;
    }
    if ((millis() - t_) > DEBOUNCE_MS && v != stable_) {
        stable_ = v;
        if (stable_ == LOW) {
            pressed_ = true;
            pressed_since_ = millis();
            edge_ = false;
            long_edge_ = false;
            long_latched_ = false;
        } else {
            // on release, emit short press only if no long press occurred
            if (pressed_ && !long_latched_) edge_ = true;
            pressed_ = false;
            long_latched_ = false;
        }
    }

    // long press detection
    if (pressed_ && !long_edge_ && !long_latched_ &&
        (millis() - pressed_since_) >= long_press_ms_) {
        long_edge_ = true;
        long_latched_ = true;
        edge_ = false;  // suppress short press for this hold
    }
}

bool Buttons::shortPress() {
    if (edge_) {
        edge_ = false;
        return true;
    }
    return false;
}

bool Buttons::longPress() {
    if (long_edge_) {
        long_edge_ = false;
        return true;
    }
    return false;
}
