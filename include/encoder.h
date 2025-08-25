#pragma once
#include <Arduino.h>

// Quadrature encoder with speed-based acceleration; produces setpoint delta in mg.
class Encoder {
public:
  void begin(uint8_t pinA, uint8_t pinB, uint8_t pinSW);
  void update();
  // Returns and clears accumulated delta (mg) since last call
  int32_t consumeDeltaMg();
  // Short press on push button (tare)
  bool tarePressed();
  // Long press on push button (enter/advance calibration)
  bool tareLongPressed();

private:
  uint8_t pa_=255, pb_=255, psw_=255;
  uint8_t prev_ = 0;
  int32_t delta_mg_ = 0;
  uint32_t lastTickMs_ = 0; float emaDt_ = 0.05f;

  // debounce & press classification for SW
  bool swLast_=true, swStable_=true; uint32_t swT_=0;
  bool swDown_=false; uint32_t swDownMs_=0; bool shortEdge_=false; bool longEdge_=false; bool longReported_=false;
};
