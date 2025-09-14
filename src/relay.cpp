#include "relay.h"

void Relay::begin(uint8_t pin) {
    pin_ = pin;
    on_ = false;
    if (pin_ != 255) {
        pinMode(pin_, OUTPUT);
        digitalWrite(pin_, LOW);
    }
}

void Relay::set(bool on) {
    on_ = on;
    if (pin_ != 255) {
        digitalWrite(pin_, on);
    }
}
