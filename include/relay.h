#pragma once
#include <Arduino.h>

class Relay {
    public:
     explicit Relay() {}

     void begin(uint8_t pin);
     void set(bool on);
     bool isOn() const { return on_; }

    private:
     uint8_t pin_ = 255;
     bool on_ = false;
};
