#include "encoder.h"

#include "config.h"
#include "utils.h"

void Encoder::begin(uint8_t pinA, uint8_t pinB, uint8_t pinSW) {
    pa_ = pinA;
    pb_ = pinB;
    psw_ = pinSW;
    pinMode(pa_, INPUT_PULLUP);
    pinMode(pb_, INPUT_PULLUP);
    pinMode(psw_, INPUT_PULLUP);
    prev_ = (digitalRead(pa_) << 1) | digitalRead(pb_);
}

void Encoder::update() {
    // --- quadrature ---
    int a = digitalRead(pa_), b = digitalRead(pb_);
    uint8_t s = (a << 1) | b;
    static const int8_t tab[4][4] = {
        {0, -1, +1, 0}, {+1, 0, 0, -1}, {-1, 0, 0, +1}, {0, +1, -1, 0}};
    int d = tab[prev_][s];
    prev_ = s;
    if (d != 0) {
        uint32_t now = millis();
        float dt = (now - lastTickMs_) / 1000.0f;
        if (dt <= 0) dt = 0.001f;
        lastTickMs_ = now;
        emaDt_ = 0.7f * emaDt_ + 0.3f * dt;
        float tps = 1.0f / emaDt_;
        float step_g = ENC_STEP_SLOW_G;
        if (tps > ENC_TPS_FAST)
            step_g = ENC_STEP_FAST_G;
        else if (tps > ENC_TPS_MED)
            step_g = ENC_STEP_MED_G;
        int32_t step_mg = lround_mg(step_g);
        delta_mg_ += (d > 0 ? +step_mg : -step_mg);

        // Serial.printf("Enc: d=%d tps=%.1f step=%.1f g dt=%.3f s\n", d, tps, step_g, dt);
    }

    // --- SW debounce & press type ---
    bool v = digitalRead(psw_);
    if (v != swLast_) {
        swT_ = millis();
        swLast_ = v;
    }
    if ((millis() - swT_) > DEBOUNCE_MS && v != swStable_) {
        swStable_ = v;
        if (swStable_ == LOW) {  // pressed
            swDown_ = true;
            swDownMs_ = millis();
            longReported_ = false;
        } else {  // released
            if (swDown_) {
                uint32_t held = millis() - swDownMs_;
                if (!longReported_ && held >= DEBOUNCE_MS) shortEdge_ = true;
            }
            swDown_ = false;
            longReported_ = false;
        }
    }

    if (swDown_ && !longReported_) {
        if (millis() - swDownMs_ >= ENC_LONGPRESS_MS) {
            longReported_ = true;
            longEdge_ = true;
        }
    }
}

int32_t Encoder::consumeDeltaMg() {
    int32_t d = delta_mg_;
    delta_mg_ = 0;
    return d;
}

bool Encoder::tarePressed() {
    if (shortEdge_) {
        shortEdge_ = false;
        return true;
    }
    return false;
}

bool Encoder::tareLongPressed() {
    if (longEdge_) {
        longEdge_ = false;
        return true;
    }
    return false;
}
