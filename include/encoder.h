#pragma once
#include <Arduino.h>
#include "buttons.h"

// Quadrature encoder with speed-based acceleration;
//   produces setpoint delta in mg.
class Encoder {
   public:
    explicit Encoder(uint32_t buttonLongPressMs);
    void begin(uint8_t pinA, uint8_t pinB, uint8_t pinSW);
    void update();
    // Returns and clears accumulated delta (mg) since last call
    int32_t consumeDeltaMg();
    // Button edges
    bool buttonShortPress();
    bool buttonLongPress();

   private:
    Buttons button_;
    uint8_t pa_ = 255, pb_ = 255, psw_ = 255;
    uint8_t prev_ = 0;
    int32_t delta_mg_ = 0;
    uint32_t lastTickMs_ = 0;
    float emaDt_ = 0.05f;
};
