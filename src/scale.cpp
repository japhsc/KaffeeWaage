#include "scale.h"

#include <limits.h>
#include <math.h>

#include "utils.h"

void Scale::begin(uint8_t dtPin, uint8_t sckPin) {
    hx_.begin(dtPin, sckPin);
    delay(400);  // settle
    ok_ = true;
    cal_q16_ = CAL_MG_PER_COUNT_Q16;  // runtime factor starts from config (Q16)
}

void Scale::tare() {
    // Capture current raw as new baseline; allows negative readings later
    tare_raw_ = weight_raw_ + tare_raw_;
}

void Scale::update() {
    uint32_t now = millis();
    if (now < tNext_) return;
    tNext_ = now + HX711_PERIOD_MS;

    if (!hx_.is_ready()) {
        ok_ = false;
        return;
    }
    ok_ = true;

    int32_t raw = hx_.read();

    // Serial.printf("Raw: %d\n", raw);

    raw -= SCALE_OFFSET_COUNTS;     // compile-time raw offset
    last_raw_no_tare_ = raw;        // save before tare
    weight_raw_ = raw - tare_raw_;  // allow negative

    buf_[bi_] = weight_raw_;
    bi_ = (bi_ + 1) % 3;
    int32_t med = med3(buf_[0], buf_[1], buf_[2]);
    // Convert counts -> mg using Q16 factor
    int32_t mg = (int32_t)(((int64_t)med * (int64_t)cal_q16_) >> 16);
    mg += SCALE_OFFSET_MG;  // optional mg-level offset

    // IIR filter: y += (x - y)/N  (alpha = 1/N)
    filt_mg_ += (mg - filt_mg_) / IIR_ALPHA_DIV;
    last_mg_ = filt_mg_;

    // ---- Stability detection ----
    if (wcount_ < STAB_WINDOW_SAMPLES) wcount_++;
    win_[widx_] = last_mg_;
    widx_ = (widx_ + 1) % STAB_WINDOW_SAMPLES;

    if (wcount_ == STAB_WINDOW_SAMPLES) {
        int64_t sum = 0, sumsq = 0;
        int32_t mn = INT32_MAX, mx = INT32_MIN;
        for (uint8_t i = 0; i < STAB_WINDOW_SAMPLES; ++i) {
            int32_t v = win_[i];
            sum += v;
            sumsq += (int64_t)v * v;
            if (v < mn) mn = v;
            if (v > mx) mx = v;
        }
        int32_t mean = (int32_t)(sum / STAB_WINDOW_SAMPLES);
        int64_t ex2 = sumsq / STAB_WINDOW_SAMPLES;
        int64_t var = ex2 - (int64_t)mean * mean;
        if (var < 0) var = 0;
        int32_t stdmg = (int32_t)sqrt((double)var);
        int32_t p2p = mx - mn;

        bool quiet = (stdmg <= STAB_STDDEV_MG) && (p2p <= STAB_P2P_MG);
        if (quiet) {
            // BUGFIX: only set stable_since_ when entering quiet state, not
            // every tick
            if (stable_since_ == 0) {
                stable_since_ = now;
            }
            stable_ = (now - stable_since_) >= STAB_DWELL_MS;
        } else {
            stable_ = false;
            stable_since_ = 0;  // reset so next quiet period starts timing
        }

        // Serial.printf("Std. dev: %d, p2p: %d, stable: %d, mg: %d\n", stdmg,
        // p2p, stable_, mg);
    } else {
        stable_ = false;  // not enough samples yet
        stable_since_ = 0;
    }
}
