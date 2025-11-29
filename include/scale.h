#pragma once
#include <Arduino.h>
#include <HX711.h>

#include "config.h"

class Scale {
   public:
    // Initialize with data pin, clock pin, and HX711
    void begin(uint8_t dtPin, uint8_t sckPin);

    // Call often; consumes samples when DRDY fires
    void update();

    // Minimum spacing between processed samples (ms) to allow decimation
    void setSamplePeriodMs(uint16_t ms);

    bool ok() const { return ok_; }
    void tare();

    // Slow/display output (smoothed)
    int32_t filteredMg() const { return filt_mg_; }

    // Fast estimator outputs (for control)
    int32_t fastMg() const { return (int32_t)x_hat_mg_; }
    float flowGps() const { return v_hat_mgps_ / 1000.0f; }  // grams per second
    float vHatMgps() const { return v_hat_mgps_; }           // mg per second
    float aHatMgps2() const { return a_hat_mgps2_; }         // mg per s^2

    // Raw
    int32_t rawCounts() const { return weight_raw_; }        // raw minus tare
    int32_t rawNoTare() const { return last_raw_no_tare_; }  // raw without tare

    // Tare persistence helpers
    int32_t tareRaw() const { return tare_raw_; }
    void setTareRaw(int32_t v) { tare_raw_ = v; }

    // Stability
    bool isStable() const { return stable_; }

    // Calibration factor at runtime (Q16 mg per count)
    void setCalMgPerCountQ16(int32_t q16) { cal_q16_ = q16; }
    int32_t calMgPerCountQ16() const { return cal_q16_; }

   private:
    static void IRAM_ATTR drdyISR();
    static Scale* instance_;

    volatile bool drdy_pending_ = false;
    uint8_t dt_pin_ = 0;

    HX711 hx_;
    bool ok_ = false;
    int32_t tare_raw_ = 0;
    int32_t weight_raw_ = 0;        // after tare
    int32_t last_raw_no_tare_ = 0;  // before tare

    // timing
    uint16_t period_ms_ = 100;
    uint32_t last_sample_ms_ = 0;

    // slow/display filter
    int32_t buf_[3] = {0, 0, 0};
    uint8_t bi_ = 0;
    int32_t filt_mg_ = 0;
    int32_t last_mg_ = 0;

    // calibration
    int32_t cal_q16_ = 1 << 16;  // mg per count in Q16

    // fast α–β estimator (position & velocity); plus EMA accel estimate
    float x_hat_mg_ = 0.0f;     // mg
    float v_hat_mgps_ = 0.0f;   // mg/s
    float a_hat_mgps2_ = 0.0f;  // mg/s^2 (EMA of dv/dt)
    const float g_ = 0.4f;      // α gain (0..1)
    const float h_ = 0.08f;     // β gain (~0..1)
    float last_v_hat_ = 0.0f;

    // Stability window
    int32_t win_[STAB_WINDOW_SAMPLES] = {0};
    uint8_t wcount_ = 0, widx_ = 0;
    bool stable_ = false;
    uint32_t stable_since_ = 0;
};
