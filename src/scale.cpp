#include "scale.h"

#include <limits.h>
#include <math.h>

#include "utils.h"

void Scale::begin(uint8_t dtPin, uint8_t sckPin) {
    hx_.begin(dtPin, sckPin);
    delay(400);  // settle
    ok_ = true;
    cal_q16_ = CAL_MG_PER_COUNT_Q16;  // runtime factor starts from config (Q16)
    setSamplePeriodMs(HX711_PERIOD_IDLE_MS);
    bootGraceUntil_ = millis() + HX711_STARTUP_GRACE_MS;
    notReadySince_ = 0;
    // rate detection init
    expected_period_ms_ = HX711_PERIOD_IDLE_MS;
    fast_capable_ = false;
    rd_count_ = 0;
    rd_accum_ms_ = 0;
    last_drdy_ms_ = 0;
}

void Scale::setSamplePeriodMs(uint16_t ms) { period_ms_ = ms; }

void Scale::tare() {
    // Capture current raw as new baseline; allows negative readings later
    tare_raw_ = weight_raw_ + tare_raw_;
}

void Scale::update() {
    uint32_t now = millis();
    if (now < tNext_) return;
    if (tPrev_ == 0) tPrev_ = now;
    uint32_t dt_ms = now - tPrev_;
    tPrev_ = now;
    tNext_ = now + period_ms_;

    // Readiness & error policy
    if (!hx_.is_ready()) {
        if (notReadySince_ == 0) notReadySince_ = now;
        // dynamic timeout based on detected/expected period
        uint32_t dynTimeout =
            (uint32_t)expected_period_ms_ * NOTREADY_MULT + NOTREADY_MARGIN_MS;
        if ((now - notReadySince_) > dynTimeout && now > bootGraceUntil_)
            ok_ = false;
        tNext_ = now + 1;  // poll soon to catch next DRDY
        return;
    }
    notReadySince_ = 0;  // DRDY seen
    ok_ = true;

    // --- Rate detection (measure DRDY intervals) ---
    if (last_drdy_ms_ != 0) {
        uint32_t d = now - last_drdy_ms_;
        if (d >= RD_MIN_MS && d <= RD_MAX_MS) {
            rd_accum_ms_ += d;
            if (rd_count_ < 255) rd_count_++;
        }
        if (rd_count_ >= RATE_DETECT_SAMPLES &&
            expected_period_ms_ == HX711_PERIOD_IDLE_MS) {
            uint32_t avg = rd_accum_ms_ / rd_count_;
            // classify as fast if closer to 13 ms than 100 ms
            uint32_t diffFast = (avg > EXPECTED_80SPS_MS)
                                    ? (avg - EXPECTED_80SPS_MS)
                                    : (EXPECTED_80SPS_MS - avg);
            uint32_t diffSlow = (avg > EXPECTED_10SPS_MS)
                                    ? (avg - EXPECTED_10SPS_MS)
                                    : (EXPECTED_10SPS_MS - avg);
            if (diffFast * 3 < diffSlow) {  // require clear win
                expected_period_ms_ = HX711_PERIOD_FAST_MS;
                fast_capable_ = true;
            } else {
                expected_period_ms_ = HX711_PERIOD_IDLE_MS;
                fast_capable_ = false;
            }
        }
    }
    last_drdy_ms_ = now;

    int32_t raw = hx_.read();
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
        double mean = (double)sum / STAB_WINDOW_SAMPLES;

        // Compute variance
        double var_sum = 0.0;
        for (uint8_t i = 0; i < STAB_WINDOW_SAMPLES; ++i) {
            double diff = win_[i] - mean;
            var_sum += diff * diff;
        }

        // Population variance (divide by N); use N-1 if you want sample
        // variance
        double var = var_sum / STAB_WINDOW_SAMPLES;
        if (var < 0.0) var = 0.0;

        // Standard deviation
        int32_t stdmg = (int32_t)std::sqrt(var);

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
