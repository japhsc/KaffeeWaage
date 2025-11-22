#include "controller.h"

#include "config.h"
#include "storage.h"
#include "utils.h"

void Controller::begin(Scale* sc, Encoder* enc, Buttons* btn, Display* disp,
                       Relay* rel) {
    sc_ = sc;
    enc_ = enc;
    btn_ = btn;
    disp_ = disp;
    rel_ = rel;
    // load learned k_v (mg per g/s)
    k_v_mg_per_gps_ = (float)storage::loadKv(0);
}

void Controller::update() {
    // --- update peripherals ---
    sc_->update();
    enc_->update();
    btn_->update();

    static uint32_t hintUntil = 0;  // transient UI hint window
    static uint32_t errSince = 0;   // debounce error display

    // track error debounce
    if (!sc_->ok()) {
        if (errSince == 0) errSince = millis();
    } else {
        errSince = 0;
    }

    // --- long-press: enter/advance calibration (requires stability) ---
    static int32_t cal_raw0 = 0;  // persistent across calls
    static uint32_t calDoneUntil = 0;

    if (enc_->tareLongPressed()) {
        if (REQUIRE_STABLE_FOR_CAL && !sc_->isStable()) {
            hintUntil = millis() + HINT_HOLD_MS;  // show HOLD
        } else if (state_ == AppState::IDLE ||
                   state_ == AppState::SHOW_SETPOINT) {
            // Capture zero point and move to span prompt
            cal_raw0 = sc_->rawNoTare();
            state_ = AppState::CAL_SPAN;
        } else if (state_ == AppState::CAL_SPAN) {
            // Capture span point, compute factor (Q16): mg_per_count_q16 =
            // (span_mg << 16) / dcounts
            int32_t raw1 = sc_->rawNoTare();
            int32_t dcounts = raw1 - cal_raw0;
            if (dcounts == 0) dcounts = 1;
            int32_t span_mg = lround_mg(CAL_SPAN_MASS_G);
            int64_t num = ((int64_t)span_mg) << 16;
            int32_t mg_per_count_q16 =
                (int32_t)(num / (int64_t)((dcounts > 0) ? dcounts : -dcounts));
            if (mg_per_count_q16 <= 0)
                mg_per_count_q16 = CAL_MG_PER_COUNT_Q16;  // fallback
            sc_->setCalMgPerCountQ16(mg_per_count_q16);
            storage::saveCalQ16(mg_per_count_q16);
            // brief done screen
            state_ = AppState::DONE_HOLD;
            calDoneUntil = millis() + DONE_HOLD_MS;
        }
    }

    // --- handle encoder setpoint ---
    int32_t dmg = enc_->consumeDeltaMg();
    if (dmg != 0) {
        int32_t maxMg = lround_mg(SETPOINT_MAX_G);
        setpoint_mg_ = clamp_i32(setpoint_mg_ + dmg, 0, maxMg);
        tShowUntil_ = millis() + SHOW_SP_MS;
        if (state_ == AppState::IDLE) state_ = AppState::SHOW_SETPOINT;
    }

    // --- save setpoint when user stops turning (on timeout exit) ---
    if (state_ == AppState::SHOW_SETPOINT && millis() > tShowUntil_) {
        state_ = AppState::IDLE;
        storage::saveSetpointMg(setpoint_mg_);
    }

    // --- tare (short press) ---
    if (enc_->tarePressed()) {
        bool measuring = (state_ == AppState::MEASURING);
        if (measuring) {
            hintUntil = millis() + HINT_HOLD_MS;  // blocked during measuring
        } else if (!REQUIRE_STABLE_FOR_TARE || sc_->isStable()) {
            sc_->tare();
            storage::saveTareRaw(sc_->tareRaw());
        } else {
            hintUntil = millis() + HINT_HOLD_MS;  // ask user to hold still
        }
    }

    // --- start/stop ---
    if (btn_->startPressed()) {
        if (state_ == AppState::MEASURING) {
            rel_->set(false);
            state_ = AppState::DONE_HOLD;
            tDoneUntil_ = millis() + DONE_HOLD_MS;
        } else if (state_ == AppState::IDLE ||
                   state_ == AppState::SHOW_SETPOINT) {
            // start
            rel_->set(true);
            // only go fast if hardware is actually 80 SPS capable
            sc_->setSamplePeriodMs(sc_->fastCapable() ? HX711_PERIOD_FAST_MS
                                                      : HX711_PERIOD_IDLE_MS);
            state_ = AppState::MEASURING;
            tMeasureUntil_ = millis() + MEASURE_TIMEOUT_MS;
        } else if (state_ == AppState::CAL_SPAN) {
            // allow abort of calibration with start button
            state_ = AppState::IDLE;
        }
    }

    // --- dynamic cutoff during measuring ---
    if (state_ == AppState::MEASURING) {
        float v = sc_->vHatMgps();   // mg/s
        float a = sc_->aHatMgps2();  // mg/s^2
        float tau = (TAU_MEAS_MS + TAU_COMM_MS + TAU_EXTRA_MS) / 1000.0f;  // s
        // dynamic offset (mg)
        float offset_dyn =
            v * tau + 0.5f * a * tau * tau + k_v_mg_per_gps_ * (v / 1000.0f);
        int32_t effective = setpoint_mg_ - (int32_t)lroundf(offset_dyn);

        if (sc_->fastMg() + HYSTERESIS_MG >= effective ||
            millis() > tMeasureUntil_) {
            // capture v at stop for learning
            last_v_stop_gps_ = sc_->flowGps();
            rel_->set(false);
            state_ = AppState::DONE_HOLD;
            tDoneUntil_ = millis() + DONE_HOLD_MS;
        }
    }

    // --- learning at end of run ---
    if (state_ == AppState::DONE_HOLD && millis() > tDoneUntil_) {
        // compute overshoot (mg) using slow/stable reading
        int32_t final_mg = sc_->filteredMg();
        int32_t eps_mg = final_mg - setpoint_mg_;
        float v = fabsf(last_v_stop_gps_);
        if (v < V_MIN_GPS) v = V_MIN_GPS;
        // Update k_v (mg per g/s) with EMA toward eps/v
        float target_kv = (float)eps_mg / v;
        k_v_mg_per_gps_ =
            (1.0f - KV_EMA_ALPHA) * k_v_mg_per_gps_ + KV_EMA_ALPHA * target_kv;
        storage::saveKv((int32_t)lroundf(k_v_mg_per_gps_));

        // reset sampling back to idle rate and return to IDLE
        sc_->setSamplePeriodMs(HX711_PERIOD_IDLE_MS);
        state_ = AppState::IDLE;
    }

    // --- display throttle & overlay ---
    if (millis() < hintUntil) {
        disp_->showHintHold();
        return;
    }

    uint32_t dispPeriod =
        (state_ == AppState::MEASURING) ? DISPLAY_MEAS_MS : DISPLAY_IDLE_MS;
    if (millis() < tDispNext_) return;
    tDispNext_ = millis() + dispPeriod;

    // --- display with error debounce ---
    bool showErr = (!sc_->ok()) && (errSince != 0) &&
                   ((millis() - errSince) > ERROR_DISPLAY_DEBOUNCE_MS);
    if (showErr) {
        disp_->showError();
    } else if (state_ == AppState::SHOW_SETPOINT) {
        disp_->showSetpointMg(setpoint_mg_);
    } else if (state_ == AppState::CAL_SPAN) {
        disp_->showCalSpan();
    } else if (state_ == AppState::DONE_HOLD && millis() < tDoneUntil_) {
        disp_->showCalDone();
    } else {  // IDLE/MEASURING
        disp_->showWeightMg(sc_->filteredMg(), sc_->isStable());
    }
}