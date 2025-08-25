#pragma once
#include <Arduino.h>
#include <HX711.h>

#include "config.h"

class Scale {
public:
  void begin(uint8_t dtPin, uint8_t sckPin);
  // Call often; samples every HX711_PERIOD_MS
  void update();
  bool ok() const { return ok_; }
  void tare();

  int32_t filteredMg() const { return filt_mg_; }
  int32_t rawCounts() const { return weight_raw_; }           // raw minus tare
  int32_t rawNoTare() const { return last_raw_no_tare_; }     // raw without tare

  // Stability
  bool isStable() const { return stable_; }
  int32_t lastMg() const { return last_mg_; }

  // Calibration factor at runtime (Q16 mg per count)
  void setCalMgPerCountQ16(int32_t q16) { cal_q16_ = q16; }
  int32_t calMgPerCountQ16() const { return cal_q16_; }

private:
  HX711 hx_;
  bool ok_ = false;
  int32_t tare_raw_ = 0;
  int32_t weight_raw_ = 0;          // after tare
  int32_t last_raw_no_tare_ = 0;    // before tare
  int32_t buf_[3] = {0,0,0};
  uint8_t  bi_ = 0;
  int32_t  filt_mg_ = 0;
  int32_t  last_mg_ = 0;
  uint32_t tNext_ = 0;
  int32_t  cal_q16_ = 1 << 16;      // mg per count in Q16

  // Stability window
  int32_t win_[STAB_WINDOW_SAMPLES] = {0};
  uint8_t wcount_ = 0, widx_ = 0;
  bool stable_ = false;
  uint32_t stable_since_ = 0;
};
