#include "buttons.h"

#include "config.h"

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
        if (stable_ == LOW) edge_ = true;
    }
}

bool Buttons::startPressed() {
    if (edge_) {
        edge_ = false;
        return true;
    }
    return false;
}
