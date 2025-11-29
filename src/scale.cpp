#include "scale.h"

#include <limits.h>
#include <math.h>

#include "utils.h"

Scale* Scale::instance_ = nullptr;

void Scale::begin(uint8_t dtPin, uint8_t sckPin) {
    dt_pin_ = dtPin;
    hx_.begin(dt_pin_, sckPin);
    delay(400);  // settle
    instance_ = this;
    attachInterrupt(digitalPinToInterrupt(dt_pin_), Scale::drdyISR, FALLING);

    ok_ = true;
    cal_q16_ = CAL_MG_PER_COUNT_Q16;  // runtime factor starts from config (Q16)
    setSamplePeriodMs(HX711_PERIOD_IDLE_MS);
    bootGraceUntil_ = millis() + HX711_STARTUP_GRACE_MS;
    last_sample_ms_ = millis();
}

void Scale::setSamplePeriodMs(uint16_t ms) { period_ms_ = (ms == 0) ? 1 : ms; }

void Scale::tare() {
    // Capture current raw as new baseline; allows negative readings later
    tare_raw_ = weight_raw_ + tare_raw_;
}

void Scale::update() {
    uint32_t now = millis();

    uint16_t timeout_base_ms =
        (period_ms_ > HX711_PERIOD_IDLE_MS) ? period_ms_ : HX711_PERIOD_IDLE_MS;
    uint32_t timeout_ms =
        (uint32_t)timeout_base_ms * NOTREADY_MULT + NOTREADY_MARGIN_MS;
    if (last_sample_ms_ != 0 && (now - last_sample_ms_) > timeout_ms &&
        now > bootGraceUntil_) {
        ok_ = false;
    }

    // wait for DRDY interrupt
    if (!drdy_pending_) return;

    // enforce requested sampling period (decimate if HX711 is faster)
    if (last_sample_ms_ != 0 && (now - last_sample_ms_) < period_ms_) return;

    drdy_pending_ = false;
    ok_ = true;

    // debug: measure actual samples per second
    /*
    static uint32_t dbg_count = 0;
    static uint32_t dbg_window_start = millis();
    dbg_count++;
    uint32_t win_ms = now - dbg_window_start;
    if (win_ms >= 1000) {  // 1 s window
        float sps = (win_ms > 0) ? (1000.0f * dbg_count / (float)win_ms) : 0.0f;
        Serial.printf("HX711 SPS: %.1f\n", sps);
        dbg_count = 0;
        dbg_window_start = now;
    }
    */

    uint32_t prev_sample_ms = last_sample_ms_;
    int32_t raw = hx_.read();
    last_sample_ms_ = now;

    raw -= SCALE_OFFSET_COUNTS;     // compile-time raw offset
    last_raw_no_tare_ = raw;        // save before tare
    weight_raw_ = raw - tare_raw_;  // allow negative

    // --- Convert counts -> mg ---
    // fast measurement (no median)
    int32_t mg_fast =
        (int32_t)(((int64_t)weight_raw_ * (int64_t)cal_q16_) >> 16);
    mg_fast += SCALE_OFFSET_MG;  // optional mg-level offset

    // slow/display path with median-of-3 + IIR
    buf_[bi_] = weight_raw_;
    bi_ = (bi_ + 1) % 3;
    int32_t med = med3(buf_[0], buf_[1], buf_[2]);
    int32_t mg_slow = (int32_t)(((int64_t)med * (int64_t)cal_q16_) >> 16);
    mg_slow += SCALE_OFFSET_MG;
    filt_mg_ += (mg_slow - filt_mg_) / IIR_ALPHA_DIV;  // IIR alpha=1/N
    last_mg_ = filt_mg_;

    // ---- Fast α–β estimator (x,v) with accel EMA ----
    uint32_t dt_ms = (prev_sample_ms == 0) ? period_ms_ : (now - prev_sample_ms);
    float dt = dt_ms / 1000.0f;
    if (dt <= 0.0001f) dt = period_ms_ / 1000.0f;
    // predict
    float x_pred = x_hat_mg_ + v_hat_mgps_ * dt;
    float v_pred = v_hat_mgps_;
    // update
    float r = (float)mg_fast - x_pred;
    x_hat_mg_ = x_pred + g_ * r;
    v_hat_mgps_ = v_pred + (h_ / dt) * r;
    // accel estimate (EMA of dv/dt)
    float dv = v_hat_mgps_ - last_v_hat_;
    float a_inst = dv / dt;
    a_hat_mgps2_ = 0.8f * a_hat_mgps2_ + 0.2f * a_inst;
    last_v_hat_ = v_hat_mgps_;

    // ---- Stability detection (on slow path) ----
    if (wcount_ < STAB_WINDOW_SAMPLES) wcount_++;
    win_[widx_] = last_mg_;
    widx_ = (widx_ + 1) % STAB_WINDOW_SAMPLES;

    if (wcount_ == STAB_WINDOW_SAMPLES) {
        int64_t sum = 0;
        int32_t mn = INT32_MAX, mx = INT32_MIN;

        // Compute sum, min, max, mean
        for (uint8_t i = 0; i < STAB_WINDOW_SAMPLES; ++i) {
            int32_t v = win_[i];
            sum += v;
            if (v < mn) mn = v;
            if (v > mx) mx = v;
        }
        float mean = (float)sum / STAB_WINDOW_SAMPLES;

        // Compute variance
        float var_sum = 0.0f;
        for (uint8_t i = 0; i < STAB_WINDOW_SAMPLES; ++i) {
            float diff = win_[i] - mean;
            var_sum += diff * diff;
        }

        // Population variance (divide by N); use N-1 if you want sample
        // variance
        float var = var_sum / STAB_WINDOW_SAMPLES;
        if (var < 0.0f) var = 0.0f;

        // Standard deviation
        int32_t stdmg = (int32_t)sqrtf(var);

        // Peak-to-peak
        int32_t p2p = mx - mn;

        // Check stability criteria
        bool quiet = (stdmg <= STAB_STDDEV_MG) && (p2p <= STAB_P2P_MG);

        // Update stable state with dwell time
        if (quiet) {
            if (stable_since_ == 0) {
                stable_since_ = now;
            }
            stable_ = (now - stable_since_) >= STAB_DWELL_MS;
        } else {
            stable_ = false;
            stable_since_ = 0;  // reset so next quiet period starts timing
        }
    } else {
        stable_ = false;  // not enough samples yet
        stable_since_ = 0;
    }
}

void IRAM_ATTR Scale::drdyISR() {
    if (instance_) instance_->drdy_pending_ = true;
}
