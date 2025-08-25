#pragma once
#include <Arduino.h>
#include <LedControl.h>

class Display {
public:
  void begin(uint8_t din, uint8_t clk, uint8_t cs);
  void showWeightMg(int32_t mg, bool stable);
  void showSetpointMg(int32_t mg);
  void showError();
  void showCalZero();
  void showCalSpan();
  void showCalDone();
  void showHintHold();
  void clear();
private:
  LedControl lc_{-1,-1,-1,1};
  int mapDigit(int d) const;
  void putChar(int pos, char c, bool dp=false);
  void putDigit(int pos, uint8_t d, bool dp=false);
  void renderNumberMg(int32_t mg);
};
