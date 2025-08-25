#pragma once
#include <Arduino.h>

#include "switch.h"

class Relay {
   public:
    explicit Relay(SwitchWorker& client, const char* ain)
        : ain_(ain), _worker(client) {}

    void begin(uint8_t pin);
    void set(bool on);
    bool isOn() const { return on_; }

   private:
    uint8_t pin_ = 255;
    bool on_ = false;
    const char* ain_;
    SwitchWorker& _worker;
};
