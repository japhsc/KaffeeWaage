#pragma once
#include <Arduino.h>

class Buttons {
   public:
    explicit Buttons(uint32_t longPressMs);
    void begin(uint8_t pinStart);
    void update();
    bool startPressed();
    bool startLongPressed();

   private:
    uint32_t long_press_ms_;
    uint8_t ps_ = 255;  // start button pin
    bool last_ = true, stable_ = true;
    uint32_t t_ = 0;
    bool edge_ = false;
    bool long_edge_ = false;
    bool pressed_ = false;
    uint32_t pressed_since_ = 0;
    bool long_latched_ = false;
};
