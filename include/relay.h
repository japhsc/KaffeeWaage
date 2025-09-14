#pragma once
#include <Arduino.h>

class Relay {
   public:
    Relay() = default;

    void begin() { count_ = 0; }

    void begin(uint8_t pin) {
        count_ = 1;
        pins_[0] = pin;
        pinMode(pin, OUTPUT);
        writeAll(false);
    }

    void begin(uint8_t pin1, uint8_t pin2) {
        count_ = 2;
        pins_[0] = pin1;
        pins_[1] = pin2;
        pinMode(pin1, OUTPUT);
        pinMode(pin2, OUTPUT);
        writeAll(false);
    }

    void set(bool on) {
        on_ = on;
        writeAll(on);
    }

    bool isOn() const { return on_; }

   private:
    void writeAll(bool on) {
        const int level = on ? HIGH : LOW;
        for (uint8_t i = 0; i < count_; ++i) {
            if (pins_[i] != kUnassigned) {
                digitalWrite(pins_[i], level);
            }
        }
    }

    static constexpr uint8_t kUnassigned = 0xFF;
    uint8_t pins_[2]{kUnassigned, kUnassigned};
    uint8_t count_ = 0;
    bool on_ = false;
};
