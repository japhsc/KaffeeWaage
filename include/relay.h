#pragma once
#include <Arduino.h>

class Relay {
public:
  void begin(uint8_t pin);
  void set(bool on); // Dummy: does not actually switch mains
  bool isOn() const { return on_; }
private:
  uint8_t pin_ = 255; bool on_ = false;
};
