#pragma once
#include <Arduino.h>

class Buttons {
public:
  void begin(uint8_t pinStart);
  void update();
  bool startPressed();
private:
  uint8_t ps_ = 255; // start button pin
  bool last_=true, stable_=true; uint32_t t_=0; bool edge_=false;
};
